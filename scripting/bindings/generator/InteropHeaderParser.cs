using System.Text.RegularExpressions;

namespace Spark.Bindings.Generator;

internal sealed record InteropParameter(string Type, string Name);

internal sealed record InteropFunction(
    string ReturnType,
    string Name,
    IReadOnlyList<InteropParameter> Parameters)
{
    public bool IsComponentApi =>
        Parameters.Count > 0 &&
        Parameters[0].Type.Contains("SparkGameComponent", StringComparison.Ordinal);
}

internal static class InteropHeaderParser
{
    private static readonly Regex ApiLine = new(
        @"SPARK_SCRIPT_API\s+(?<ret>[\w\s\*]+?)\s+(?<name>spark_\w+)\s*\((?<args>[^;]*)\)\s*;",
        RegexOptions.Compiled);

    public static IReadOnlyList<InteropFunction> ParseFile(string headerPath)
    {
        var text = File.ReadAllText(headerPath);
        text = Regex.Replace(text, @"\r\n", "\n");
        text = Regex.Replace(text, @"\n\s+", " ");
        var list = new List<InteropFunction>();
        foreach (Match m in ApiLine.Matches(text))
        {
            var ret = NormalizeType(m.Groups["ret"].Value);
            var name = m.Groups["name"].Value.Trim();
            var args = m.Groups["args"].Value.Trim();
            if (string.IsNullOrEmpty(args))
            {
                list.Add(new InteropFunction(ret, name, []));
                continue;
            }

            var parameters = ParseParameters(args);
            list.Add(new InteropFunction(ret, name, parameters));
        }

        return list;
    }

    private static List<InteropParameter> ParseParameters(string args)
    {
        var parts = SplitTopLevel(args, ',');
        var parameters = new List<InteropParameter>();
        foreach (var part in parts)
        {
            var trimmed = part.Trim();
            if (trimmed.Length == 0)
            {
                continue;
            }

            var lastSpace = trimmed.LastIndexOf(' ');
            if (lastSpace <= 0)
            {
                parameters.Add(new InteropParameter(trimmed, ""));
                continue;
            }

            var type = NormalizeType(trimmed[..lastSpace].Trim());
            var name = trimmed[(lastSpace + 1)..].Trim();
            parameters.Add(new InteropParameter(type, name));
        }

        return parameters;
    }

    private static string NormalizeType(string type)
    {
        type = Regex.Replace(type, @"\s+", " ").Trim();
        type = type.Replace("const ", "", StringComparison.Ordinal);
        return type.Trim();
    }

    private static List<string> SplitTopLevel(string s, char sep)
    {
        var result = new List<string>();
        var depth = 0;
        var start = 0;
        for (var i = 0; i < s.Length; i++)
        {
            var c = s[i];
            if (c is '(' or '[')
            {
                depth++;
            }
            else if (c is ')' or ']')
            {
                depth--;
            }
            else if (c == sep && depth == 0)
            {
                result.Add(s[start..i]);
                start = i + 1;
            }
        }

        result.Add(s[start..]);
        return result;
    }
}
