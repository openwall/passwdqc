#ifdef ENABLE_NLS
#include <libintl.h>
#define _(msgid) dgettext(PACKAGE, msgid)
#define P2_(msgid, count) (dngettext(PACKAGE, (msgid), (msgid), (count)))
#define P3_(msgid, msgid_plural, count) (dngettext(PACKAGE, (msgid), (msgid_plural), (count)))
#define N_(msgid) msgid
#else
#define _(msgid) (msgid)
#define P2_(msgid, count) (msgid)
#define P3_(msgid, msgid_plural, count) ((count) == 1 ? (msgid) : (msgid_plural))
#define N_(msgid) msgid
#endif
