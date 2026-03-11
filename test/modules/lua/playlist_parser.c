#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_lua_playlist_parser
#undef VLC_DYNAMIC_PLUGIN

#include "../../libvlc/test.h"

#include <vlc/vlc.h>

#include <vlc_common.h>
#include <vlc_demux.h>
#include <vlc_extensions.h>
#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <vlc_modules.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_plugin.h>

#include <limits.h>

#include "../lib/libvlc_internal.h"

const char vlc_module_name[] = MODULE_STRING;

static int exitcode = 0;

static int OpenIntf(vlc_object_t *root) {
  setenv("VLC_USERDATA_PATH", TOP_SRCDIR "/test/modules/", 1);

  input_item_t *item = input_item_New(
      "file://" TOP_SRCDIR "/test/modules/lua/test.txt", "lua_test_sample");
  assert(item);

  stream_t *s = vlc_stream_NewURL(root, item->psz_uri);

  if (s == NULL) {
    input_item_Release(item);
    return 1;
  }

  demux_t *demuxer_p = demux_New(root, "lua", item->psz_uri, s, NULL);

  if (demuxer_p == NULL) {
    vlc_stream_Delete(s);
    input_item_Release(item);
    exitcode = 77;
    return 1;
  }

  input_item_node_t *node = input_item_node_Create(item);
  int read_dir = vlc_stream_ReadDir(demuxer_p, node);

  assert(read_dir == 0);

  if (node->i_children > 0) {
    input_item_t *child = node->pp_children[0]->p_item;
    assert(strcmp(child->psz_uri, "test://www.testvideos.com/test.mp4") == 0);
  }

  input_item_node_Delete(node);
  input_item_Release(item);
  demux_Delete(demuxer_p);

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
