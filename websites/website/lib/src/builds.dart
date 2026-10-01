import 'flashdocs.dart';

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

  /// The main full-stack image asset for [device], if present.
  BuildAsset? fullImageFor(FlashDocDevice device) =>
      _named(device.fullImageAssets);

  /// The clean GSI-only image asset for [device], if present.
  BuildAsset? cleanImageFor(FlashDocDevice device) =>
      _named(device.cleanImageAssets);

  /// First asset matching one of [names], in preference order - later
  /// entries are spellings older releases still carry.
  BuildAsset _named(List<String> names) {
    for (final n in names) {
      for (final a in assets) {
        if (a.name == n) return a;
      }
    }
    return const BuildAsset('', '');
  }
}

/// Tag prefixes that belong to desktop-app release lanes, not OS images.
/// The releases API returns every lane mixed together - the downloads
/// page for OS images must skip these.
const appTagPrefixes = ['cte-', 'hbsup-'];

bool isAppTag(String tag) =>
    appTagPrefixes.any((p) => tag.startsWith(p));

/// OS image releases only - app lanes (cte-v*, hbsup-v*) are blacklisted.
List<BuildRelease> osReleases(List<BuildRelease> all) =>
    all.where((r) => !isAppTag(r.tag)).toList();

/// One app lane's releases (tag prefix like `cte-`), newest first.
List<BuildRelease> appReleases(List<BuildRelease> all, String prefix) =>
    (all.where((r) => r.tag.startsWith(prefix)).toList()
          ..sort((a, b) => b.createdAt.compareTo(a.createdAt)));

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

/// The newest release in one channel, or null when the lane is empty.
BuildRelease? latestInChannel(List<BuildRelease> all, BuildChannel ch) {
  final builds = releasesInChannel(all, ch);
  return builds.isEmpty ? null : builds.first;
}
