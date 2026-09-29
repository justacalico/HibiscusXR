# keyboard

The system input method, and the only one in the image - LatinIME is
stripped by `tools/build/410_strip_stock_apps.sh`, and `pn2-settings.rc`
bakes this service in as `default_input_method`.

Plain Java, no Gradle: `make apk` produces `out/pn2keyboard.apk`
(javac -> d8 -> aapt2 -> zipalign -> apksigner, same pipeline as vrhome).

## Why it doesn't dock

Android 10 lets an IME window live on the target's own display or the
default display - a docked strip inside the app panel is the best the IME
window itself can do. To float Quest-style, the keyboard renders
somewhere else entirely: the HUD owns a `Surface`/`SurfaceTexture` pair,
hands the `Surface` over on request
(`gitlab.neosalsa.keyboard.action.QUERY` -> `...action.SURFACE`), and this
service wraps it in a private virtual display (`pn2kbd`) hosting the keys
as a `Presentation`. Private + same-uid owner is the only combination
window manager accepts for presentation windows, which is why the display
splits that way: HUD keeps the texture, this app keeps the display.

While the quad is up the service reports it
(`gitlab.neosalsa.hud.action.KBD`, `shown` + `display` extras) so the HUD
draws the texture under the panel that owns the field and injects taps
onto the reported display. The docked input view collapses to a hairline.
If the surface never shows up the docked view stays a real keyboard - the
fallback keeps text entry possible even with the HUD's half missing.

## Keys

Stock `KeyboardView` over `res/xml/kbd_qwerty.xml` / `kbd_symbols.xml`:
letters, digits, shift, symbols page, delete (hold-repeat), enter, and a
hide key. Everything is `commitText`/`sendKeyEvent` on the bound
`InputConnection`, so it types into whatever display the field sits on.
