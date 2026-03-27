#include <assert.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_lua_search_discovery
#undef VLC_DYNAMIC_PLUGIN

#include "../../libvlc/test.h"

#include <vlc/vlc.h>

#include <vlc_common.h>
#include <vlc_extensions.h>
#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <vlc_modules.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_plugin.h>
#include <vlc_services_discovery.h>

#include <limits.h>

#include "../lib/libvlc_internal.h"

const char vlc_module_name[] = MODULE_STRING;

static int OnLuaEventTriggered(vlc_object_t *obj, const char *name,
                               vlc_value_t oldv, vlc_value_t newv,
                               void *opaque) {
  (void)obj;
  (void)name;
  (void)oldv;
  (void)newv;
  vlc_sem_t *sem = opaque;
  vlc_sem_post(sem);
  return VLC_SUCCESS;
}

static int search_sd(services_discovery_t *sd_p, ...) {
  va_list args;
  va_start(args);

  int ret = vlc_sd_control(sd_p, SD_CMD_SEARCH, args);

  va_end(args);
  return ret;
}

static int OpenIntf(vlc_object_t *root) {
  int exit_code = VLC_SUCCESS;
  vlc_object_t *libvlc = (vlc_object_t *)vlc_object_instance(root);
  var_Create(libvlc, "test-lua-main", VLC_VAR_STRING | VLC_VAR_ISCOMMAND);
  var_Create(libvlc, "test-lua-search", VLC_VAR_STRING | VLC_VAR_ISCOMMAND);

  setenv("VLC_USERDATA_PATH", TOP_SRCDIR "/test/modules/", 1);

  vlc_sem_t sem_main, sem_search;
  vlc_sem_init(&sem_main, 0);
  vlc_sem_init(&sem_search, 0);

  const struct services_discovery_owner_t owner = {
      .cbs = NULL,
      .sys = NULL,
  };

  // based on config chain : "module{a=b}:module2{name=value}";
  // we could even pass it in libvlc flag --lua-sd
  services_discovery_t *sd_p = vlc_sd_Create(libvlc, "luasd{sd=test}", &owner);
  assert(sd_p);

  var_AddCallback(libvlc, "test-lua-main", OnLuaEventTriggered, &sem_main);
  var_AddCallback(libvlc, "test-lua-search", OnLuaEventTriggered, &sem_search);
  vlc_sem_wait(&sem_main);
  //  triggering search
  int ret = search_sd(sd_p, "test");
  if (ret != VLC_SUCCESS) {
    exit_code = VLC_EGENERIC;
  } else {
    vlc_sem_wait(&sem_search);
  }

  var_DelCallback(libvlc, "test-lua-main", OnLuaEventTriggered, &sem_main);
  var_DelCallback(libvlc, "test-lua-search", OnLuaEventTriggered, &sem_search);
  vlc_sd_Destroy(sd_p);

  return exit_code;
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
