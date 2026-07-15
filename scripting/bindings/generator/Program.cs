using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace Spark.Bindings.Generator;

/// <summary>
/// Runs <c>ClangSharpPInvokeGenerator</c> against <c>bindings.config.json</c>.
/// </summary>
internal static class Program
{
    private sealed class BindingsConfig
    {
        public string OutputPath { get; set; } = "generated/Spark.Bindings";
        public string Namespace { get; set; } = "Spark.Bindings";
        public string LibraryName { get; set; } = "SparkInterop";
        public List<string> Headers { get; set; } = [];
        public List<string> IncludeDirectories { get; set; } = [];
        public List<string> ClangArgs { get; set; } = [];
        public List<string> Exclusions { get; set; } = [];
        public string? CompileCommands { get; set; }
    }

    public static int Main(string[] args)
    {
        var configPath = args.Length > 0
            ? Path.GetFullPath(args[0])
            : Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "bindings.config.json"));

        if (!File.Exists(configPath))
        {
            Console.Error.WriteLine($"Config not found: {configPath}");
            return 1;
        }

        var configDir = Path.GetDirectoryName(configPath)!;
        var config = JsonSerializer.Deserialize<BindingsConfig>(
                         File.ReadAllText(configPath),
                         new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
                     ?? throw new InvalidOperationException("Invalid bindings.config.json");

        var outputDir = Path.GetFullPath(Path.Combine(configDir, config.OutputPath));
        Directory.CreateDirectory(outputDir);

        var headerPaths = config.Headers
            .Select(p => Path.GetFullPath(Path.Combine(configDir, p)))
            .Where(File.Exists)
            .ToList();

        if (headerPaths.Count == 0)
        {
            Console.Error.WriteLine("No headers found — check paths in bindings.config.json");
            return 1;
        }

        var includeDirs = config.IncludeDirectories
            .Select(p => Path.GetFullPath(Path.Combine(configDir, p)))
            .ToList();

        AddPlatformIncludeDirectories(includeDirs);

        var headerDir = Path.GetDirectoryName(headerPaths[0])!;
        var nativeOut = Path.Combine(outputDir, "Native.g.cs");

        var cliArgs = BuildClangSharpArgs(config, headerPaths, headerDir, includeDirs, nativeOut);

        var libClangDir = ResolveLibClangDirectory();
        if (libClangDir is null)
        {
            Console.Error.WriteLine(
                "ClangSharpPInvokeGenerator not found. Install: dotnet tool restore (repo .config/dotnet-tools.json)");
            return 1;
        }

        var psi = new ProcessStartInfo
        {
            FileName = "ClangSharpPInvokeGenerator",
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
        };

        if (OperatingSystem.IsMacOS() || OperatingSystem.IsLinux())
        {
            var key = OperatingSystem.IsMacOS() ? "DYLD_LIBRARY_PATH" : "LD_LIBRARY_PATH";
            var existing = Environment.GetEnvironmentVariable(key);
            psi.Environment[key] = string.IsNullOrEmpty(existing) ? libClangDir : $"{libClangDir}{Path.PathSeparator}{existing}";
        }

        foreach (var a in cliArgs)
        {
            psi.ArgumentList.Add(a);
        }

        using var process = Process.Start(psi);
        if (process is null)
        {
            Console.Error.WriteLine("Failed to start ClangSharpPInvokeGenerator");
            return 1;
        }

        var stdout = process.StandardOutput.ReadToEnd();
        var stderr = process.StandardError.ReadToEnd();
        process.WaitForExit();

        if (!string.IsNullOrWhiteSpace(stdout))
        {
            Console.WriteLine(stdout);
        }
        if (!string.IsNullOrWhiteSpace(stderr))
        {
            Console.Error.WriteLine(stderr);
        }

        // ClangSharp may exit 122 when emitting visibility warnings for SPARK_SCRIPT_API; output is still valid.
        if (!File.Exists(nativeOut))
        {
            Console.Error.WriteLine($"ClangSharpPInvokeGenerator failed (exit {process.ExitCode}); {nativeOut} missing");
            return process.ExitCode != 0 ? process.ExitCode : 1;
        }

        if (process.ExitCode is not 0 and not 122 and not 133 and not 134)
        {
            Console.Error.WriteLine($"ClangSharpPInvokeGenerator failed (exit {process.ExitCode})");
            return process.ExitCode;
        }

        PrependNativeFileHeader(nativeOut);
        Console.WriteLine($"Generated P/Invoke -> {nativeOut}");
        return 0;
    }

    private static List<string> BuildClangSharpArgs(
        BindingsConfig config,
        List<string> headerPaths,
        string headerDir,
        List<string> includeDirs,
        string nativeOut)
    {
        var cliArgs = new List<string>
        {
            "-o", nativeOut,
            "-n", config.Namespace,
            "-l", config.LibraryName,
            "-m", "Native",
            "-x", "c",
            "-std", "c11",
            "-F", headerDir,
            "-c", "generate-disable-runtime-marshalling",
            "-c", "compatible-codegen",
        };

        foreach (var header in headerPaths)
        {
            cliArgs.Add("-f");
            cliArgs.Add(Path.GetFileName(header));
        }

        foreach (var inc in includeDirs)
        {
            cliArgs.Add("--include-directory");
            cliArgs.Add(inc);
        }

        foreach (var extra in config.ClangArgs)
        {
            cliArgs.Add(extra);
        }

        foreach (var exclusion in config.Exclusions)
        {
            cliArgs.Add("-e");
            cliArgs.Add(exclusion);
        }

        return cliArgs;
    }

    private static void AddPlatformIncludeDirectories(List<string> includeDirs)
    {
        if (OperatingSystem.IsMacOS())
        {
            try
            {
                var sdk = RunCapture("xcrun", "--sdk macosx --show-sdk-path").Trim();
                if (!string.IsNullOrEmpty(sdk))
                {
                    includeDirs.Add(Path.Combine(sdk, "usr", "include"));
                }
            }
            catch
            {
                // xcrun unavailable — ClangSharp may still work with explicit -I paths in config.
            }
        }
        else if (OperatingSystem.IsLinux())
        {
            foreach (var candidate in new[] { "/usr/include", "/usr/local/include" })
            {
                if (Directory.Exists(candidate))
                {
                    includeDirs.Add(candidate);
                }
            }
        }
    }

    private static string? ResolveLibClangDirectory()
    {
        var home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        var rid = RuntimeInformation.ProcessArchitecture switch
        {
            Architecture.X64 => OperatingSystem.IsMacOS() ? "osx-x64" : "linux-x64",
            Architecture.Arm64 => OperatingSystem.IsMacOS() ? "osx-arm64" : "linux-arm64",
            _ => null,
        };
        if (rid is null)
        {
            return null;
        }

        var storeRoot = Path.Combine(home, ".dotnet", "tools", ".store", "clangsharppinvokegenerator");
        if (!Directory.Exists(storeRoot))
        {
            return null;
        }

        var versionDir = Directory.GetDirectories(storeRoot).OrderByDescending(static d => d).FirstOrDefault();
        if (versionDir is null)
        {
            return null;
        }

        var nativeDir = Path.Combine(
            versionDir,
            $"clangsharppinvokegenerator.{rid}",
            Path.GetFileName(versionDir),
            "tools",
            "any",
            rid);

        return Directory.Exists(nativeDir) ? nativeDir : null;
    }

    private static void PrependNativeFileHeader(string nativeOut)
    {
        var body = File.ReadAllText(nativeOut);
        body = System.Text.RegularExpressions.Regex.Replace(
            body,
            @"\s*\[return:\s*NativeTypeName\(""[^""]*""\)\]",
            string.Empty);
        body = System.Text.RegularExpressions.Regex.Replace(
            body,
            @"\s*\[NativeTypeName\(""[^""]*""\)\]",
            string.Empty);

        const string header = """
            // <auto-generated /> — ClangSharpPInvokeGenerator from SparkInterop.h / SparkInteropTypes.h
            // Regenerate: ./tools/generate-csharp-bindings.sh
            using System.Runtime.InteropServices;

            """;

        var namespaceIndex = body.IndexOf("namespace Spark.Bindings", StringComparison.Ordinal);
        if (namespaceIndex >= 0)
        {
            body = header + body[namespaceIndex..];
        }
        else
        {
            body = header + body.TrimStart();
        }

        File.WriteAllText(nativeOut, body);
    }

    private static string RunCapture(string fileName, string arguments)
    {
        var psi = new ProcessStartInfo(fileName, arguments)
        {
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
        };
        using var p = Process.Start(psi) ?? throw new InvalidOperationException($"Failed to start {fileName}");
        var output = p.StandardOutput.ReadToEnd();
        p.WaitForExit();
        return output;
    }
}
