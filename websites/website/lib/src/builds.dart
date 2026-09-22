/// Release channel for a dist build.
enum BuildChannel { release, beta, alpha }

/// One downloadable asset inside a release.
class BuildAsset {
  const BuildAsset(this.name, this.url);

  final String name;
  final String url;

  bool get isImage => name.endsWith('.img.xz');
}

/// One release published by the dist pipeline.
class BuildRelease {
  const BuildRelease(
    this.tag,
    this.name,
    this.createdAt,
    this.channel,
    this.assets,
  );

  final String tag;
  final String name;
  final DateTime createdAt;
  final BuildChannel channel;
  final List<BuildAsset> assets;

  /// The main full-stack image asset, if present.
  BuildAsset? get fullImage =>
      _named(const ['system-hibiscus-full.img.xz', 'system-pn2-full.img.xz']);

  /// The clean GSI-only image asset, if present.
  BuildAsset? get cleanImage =>
      _named(const ['system-hibiscus.img.xz', 'system-pn2.img.xz']);

  /// First asset matching one of [names], in preference order. The second
  /// entry is the pre-rename spelling older releases still carry.
  BuildAsset _named(List<String> names) {
    for (final n in names) {
      for (final a in assets) {
        if (a.name == n) return a;
      }
    }
    return const BuildAsset('', '');
  }
}

/// Determines the channel from a tag name.
///
/// release: `v2026.09.15-r6`
/// beta:    `beta-v2026.09.15-r6`
/// alpha:   `alpha-v2026.09.15-r6`
BuildChannel channelOfTag(String tag) {
  if (tag.startsWith('alpha-')) return BuildChannel.alpha;
  if (tag.startsWith('beta-')) return BuildChannel.beta;
  return BuildChannel.release;
}

/// Parses the GitLab releases API response (a list of release objects).
List<BuildRelease> parseReleases(List<dynamic> json) {
  return [
    for (final r in json)
      if (r is Map<String, dynamic>)
        BuildRelease(
          r['tag_name'] as String,
          (r['name'] as String?) ?? r['tag_name'] as String,
          DateTime.parse(r['created_at'] as String),
          channelOfTag(r['tag_name'] as String),
          [
            for (final a in ((r['assets']?['links']) as List?) ?? <Object>[])
              if (a is Map<String, dynamic>)
                BuildAsset(
                  a['name'] as String,
                  a['url'] as String,
                ),
          ],
        ),
  ];
}

/// Releases in one channel, newest first.
List<BuildRelease> releasesInChannel(
        List<BuildRelease> all, BuildChannel ch) =>
    (all.where((r) => r.channel == ch).toList()
          ..sort((a, b) => b.createdAt.compareTo(a.createdAt)));
