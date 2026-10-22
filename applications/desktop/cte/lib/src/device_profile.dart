/// What cte knows about one supported target: its display name and which
/// on-device props and log tags its driver's feeds use. Transports and
/// parsers resolve a profile off getprop output and stay generic - adding
/// support for a new headset is one entry here, not edits through the app.
class DeviceProfile {
  const DeviceProfile({
    required this.ids,
    required this.label,
    required this.poseDumpProp,
    required this.dofProps,
    required this.poseLogTags,
  });

  /// `ro.product.device` / `ro.product.name` values that select this
  /// profile.
  final Set<String> ids;

  /// Display name for the headset; empty falls back to the raw model.
  final String label;

  /// Prop that turns the driver's pose dump on.
  final String poseDumpProp;

  /// Props carrying the user tracking-mode setting; first set wins.
  final List<String> dofProps;

  /// Logcat tags that carry pose lines the [PoseLogParser] understands.
  final List<String> poseLogTags;
}

/// Fallback for hardware with no entry: the neutral prop/log names every
/// driver is expected to speak.
const kGenericDeviceProfile = DeviceProfile(
  ids: {},
  label: '',
  poseDumpProp: 'debug.hibiscus.posedump',
  dofProps: ['persist.hibiscus.dof'],
  poseLogTags: ['hibiscuspose'],
);

/// Every supported target, resolved against device identity props.
const kDeviceProfiles = <DeviceProfile>[
  DeviceProfile(
    ids: {'PICOA7B10', 'A7B10'},
    label: 'Pico Neo 2',
    poseDumpProp: 'debug.pn2.posedump',
    dofProps: ['persist.pn2.dof', 'persist.hibiscus.dof'],
    poseLogTags: ['pn2pose', 'hibiscuspose'],
  ),
];

DeviceProfile? _match(String? id) {
  if (id == null || id.isEmpty) return null;
  for (final p in kDeviceProfiles) {
    if (p.ids.contains(id)) return p;
  }
  return null;
}

/// Profile for a device identity string (`ro.product.device`).
DeviceProfile profileForDevice(String id) =>
    _match(id) ?? kGenericDeviceProfile;

/// Profile for a getprop table: device prop first, product name second.
DeviceProfile profileForProps(Map<String, String> props) =>
    _match(props['ro.product.device']) ??
    _match(props['ro.product.name']) ??
    kGenericDeviceProfile;
