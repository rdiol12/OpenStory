using LibProsperoPkg;
using LibProsperoPkg.PKG;
using LibProsperoPkg.PFS;
using System.Text;
using System.Text.Json;

// Fail before a long build if the local dependency truncates offsets above 4 GiB.
foreach (ulong offset in new ulong[] { 0xFFFF0000, 0x100000000, 0x112340000 }) {
    var bytes = new ProsperoPs5InnerMetadata(0, 0).Build(
        new[] { new ProsperoPs5MetaNode { LogicalOffset = offset, Afid = 7,
            ParentInode = 3, DirentOffset = 24 } }, 0x20000, Array.Empty<byte[]>());
    using var stream = new MemoryStream(bytes);
    stream.Position = ProsperoPs5InnerMetadata.BlockSize;
    var inode = DinodePpr.ReadFromStream(stream);
    if ((ulong)inode.DataOffset != offset || BitConverter.ToInt32(inode.Tail, 0) != 7 ||
        BitConverter.ToInt32(inode.Tail, 4) != 3 || BitConverter.ToInt32(inode.Tail, 8) != 24)
        throw new InvalidDataException("LibProsperoPkg truncates 64-bit inode offsets; apply the OpenStory metadata fix.");
}
if (args.Length == 1 && args[0] == "--check") {
    var napsMetadata = ProsperoNapsMeta.DecryptMeta18(ProsperoNapsMeta.BuildMeta18(
        0x30000, new byte[0x10000], Array.Empty<(string, long)>()));
    if (BitConverter.ToUInt32(napsMetadata, 16 + 12) != 0x40000)
        throw new InvalidDataException("NAPS phdr must declare 256-KiB mapping blocks, independent of total image size.");
    var incompressible = new byte[0x80000];
    new Random(9).NextBytes(incompressible.AsSpan(0, 0x40000));
    var inner = new ProsperoPs5InnerImageAssembler(0, 0).Build([
        new ProsperoPs5InnerFile { Path = "/data.bin", Data = incompressible }
    ]);
    var mappedMetadata = ProsperoNapsMeta.DecryptMeta18(ProsperoNapsMeta.BuildMeta18(
        (ulong)inner.ImageLength, new byte[0x10000],
        new[] { ("data.bin", incompressible.LongLength), ("*PFSmetadata", inner.MetadataPlaintext.LongLength) }, inner));
    int filesOffset = 0, blocksOffset = 0;
    for (int offset = 0; offset < mappedMetadata.Length;) {
        string tag = Encoding.ASCII.GetString(mappedMetadata, offset, 4);
        if (tag == "elif") filesOffset = offset + 16;
        if (tag == "bo2i") blocksOffset = offset + 16;
        offset += 16 + checked((int)BitConverter.ToUInt64(mappedMetadata, offset + 8));
    }
    int metadataBlock = checked((int)BitConverter.ToUInt32(mappedMetadata, filesOffset + 24 + 8));
    if (filesOffset == 0 || blocksOffset == 0 ||
        BitConverter.ToUInt32(mappedMetadata, blocksOffset + metadataBlock * 40 + 36) == 0x40110000)
        throw new InvalidDataException("NAPS metadata-file index points to a padding hole instead of the actual metadata.");
    if (inner.Placements[0].CompressionBlocks?[0].IsStored != true ||
        BitConverter.ToUInt32(mappedMetadata, blocksOffset + 16) != 0x20000 ||
        BitConverter.ToUInt32(mappedMetadata, blocksOffset + 20) != 0x20000 ||
        BitConverter.ToUInt32(mappedMetadata, blocksOffset + 36) != 0x40990000)
        throw new InvalidDataException("NAPS stored fallback must use two raw 128-KiB halves, not a compressed 256-KiB half.");
    Console.WriteLine("PASS: inode offsets and adjacent metadata survive the 4-GiB boundary.");
    Console.WriteLine("PASS: NAPS mapping-block size matches the publisher reference metadata.");
    Console.WriteLine("PASS: NAPS metadata-file index skips padding holes.");
    Console.WriteLine("PASS: incompressible NAPS blocks retain their raw fallback geometry.");
    return;
}
if (args.Length != 2)
    throw new ArgumentException("Expected prepared OpenStory app folder and output directory.");
using var metadata = JsonDocument.Parse(File.ReadAllText(Path.Combine(args[0], "sce_sys", "param.json")));
const string titleId = "PPSA99783", contentId = "UP9000-PPSA99783_00-OPENSTORYPS50000";
if (metadata.RootElement.GetProperty("titleId").GetString() != titleId ||
    metadata.RootElement.GetProperty("contentId").GetString() != contentId)
    throw new ArgumentException("This packager only builds the OpenStory title.");
// The pinned library knows these media IDs but omits them from its PFS exclusion map.
EntryNames.NameToId["pic2.png"] = (EntryId)0x2040;
EntryNames.NameToId["pic2.dds"] = (EntryId)0x2060;
EntryNames.NameToId["playgo-scenario.json"] = (EntryId)0x3000;
var result = ProsperoPackageBuilder.Build(new ProsperoBuildOptions {
    SourceFolder = args[0], OutputFolder = args[1], TitleId = titleId, ContentId = contentId,
    Title = "OpenStory", Version = "01.00", GenerateParamJsonIfMissing = false,
    OutputFormat = ProsperoOutputFormat.DebugImage, UsePublisherPprNaps = true,
    EncryptOuterPfs = false, OuterPfsSeed = Encoding.ASCII.GetBytes("PPRPLAIN-NOAUTH!"),
    RequirePublisherCompatibility = true, TimeStamp = DateTime.UtcNow
}, Console.WriteLine);
foreach (var warning in result.Warnings) Console.Error.WriteLine(warning);
Console.WriteLine(result.OutputPath);
