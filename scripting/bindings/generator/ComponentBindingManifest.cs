using System.Text.Json;
using System.Text.Json.Serialization;

namespace Spark.Bindings.Generator;

internal sealed class PrefixBinding
{
    [JsonPropertyName("class")]
    public string Class { get; set; } = "";

    [JsonPropertyName("namespace")]
    public string Namespace { get; set; } = "Spark.Bindings.Components";

    [JsonPropertyName("base")]
    public string Base { get; set; } = "GameComponentHandle";
}

internal sealed class ComponentBindingManifest
{
    [JsonPropertyName("skipMirrorCodegenClasses")]
    public List<string> SkipMirrorCodegenClasses { get; set; } = [];

    [JsonPropertyName("classOverrides")]
    public Dictionary<string, string> ClassOverrides { get; set; } = new();

    [JsonPropertyName("prefixToClass")]
    public Dictionary<string, PrefixBinding> PrefixToClass { get; set; } = new();

    public static ComponentBindingManifest Load(string path)
    {
        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<ComponentBindingManifest>(json,
                   new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
               ?? throw new InvalidOperationException($"Invalid manifest: {path}");
    }

    public string? MatchPrefix(string functionName)
    {
        if (!functionName.StartsWith("spark_", StringComparison.Ordinal))
        {
            return null;
        }

        var rest = functionName["spark_".Length..];
        var ordered = PrefixToClass.Keys.OrderByDescending(static k => k.Length);
        foreach (var prefix in ordered)
        {
            if (rest.StartsWith(prefix + "_", StringComparison.Ordinal))
            {
                return prefix;
            }
        }

        return null;
    }

}
