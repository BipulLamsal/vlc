#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_lua_art_fetcher
#undef VLC_DYNAMIC_PLUGIN

#include "../../libvlc/test.h"

#include <vlc/vlc.h>

#include <limits.h>
#include <vlc_common.h>
#include <vlc_extensions.h>
#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <vlc_modules.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_plugin.h>
#include <vlc_preparser.h>

#include "../lib/libvlc_internal.h"

const char vlc_module_name[] = MODULE_STRING;

static int exitcode = 0;

static void on_ended(vlc_preparser_req *req, int status, void *opaque) {
  (void)req;
  vlc_sem_t *sem = opaque;
  vlc_sem_post(sem);
}

static int OpenIntf(vlc_object_t *root) {
  vlc_object_t *libvlc = (vlc_object_t *)vlc_object_instance(root);

  setenv("VLC_USERDATA_PATH", TOP_SRCDIR "/test/modules/", 1);

  vlc_sem_t sem_fetched;
  vlc_sem_init(&sem_fetched, 0);

  input_item_t *item =
      input_item_New("mock://length=100000000000000000", "lua_test_sample");

  struct vlc_preparser_cbs cb = {.on_ended = on_ended};
  struct vlc_preparser_cfg cfg = {.types = VLC_PREPARSER_TYPE_FETCHMETA_ALL};

  vlc_preparser_t *preparser = vlc_preparser_New(libvlc, &cfg);

  if (!preparser) {
    input_item_Release(item);
    return VLC_EGENERIC;
  }

  vlc_preparser_Push(preparser, item, VLC_PREPARSER_TYPE_FETCHMETA_ALL, &cb,
                     &sem_fetched);

  vlc_sem_wait(&sem_fetched);

  vlc_preparser_Delete(preparser);
  return VLC_SUCCESS;
}

/** Inject the mocked modules as a static plugin: **/
vlc_module_begin() set_callback(OpenIntf) set_capability("interface", 0)
    vlc_module_end()

        VLC_EXPORT const vlc_plugin_cb vlc_static_modules[] = {
            VLC_SYMBOL(vlc_entry), NULL};

int main(void) {
  test_init();

  const char *const args[] = {
      "-vvv",
      "--vout=dummy",
      "--aout=dummy",
      "--text-renderer=dummy",
      "--no-auto-preparse",
  };

  libvlc_instance_t *vlc = libvlc_new(ARRAY_SIZE(args), args);

  libvlc_InternalAddIntf(vlc->p_libvlc_int, MODULE_STRING);
  libvlc_InternalPlay(vlc->p_libvlc_int);

  libvlc_release(vlc);
  return 0;
}
