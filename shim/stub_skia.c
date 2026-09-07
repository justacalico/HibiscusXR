// Empty stand-in for libskia.so.
//
// libaircamera.so lists libskia.so in DT_NEEDED but imports ZERO symbols from it,
// so the real library is dead weight. That matters because 8.1's libskia drags
// ICU 60, and Q's libandroidicu.so needs ICU 63 in the same process - an
// irreconcilable conflict that no LD_LIBRARY_PATH trick can isolate.
//
// Satisfying the dependency with an empty library drops the whole chain. If
// something ever does call into skia it will fail to resolve at load time, loudly,
// rather than silently misbehave.
//
// Built with -soname libskia.so so the loader accepts it in place of the original.
static const char pn2_stub_note[] =
    "pn2: empty libskia stand-in; libaircamera imports no skia symbols";

const char *pn2_skia_stub_note(void) { return pn2_stub_note; }
