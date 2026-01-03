#include "fs.h"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

EM_ASYNC_JS(void, js_sync_fs, (void), {
	await new Promise((resolve) => {
		FS.syncfs(false, resolve);
	});
})

EM_ASYNC_JS(void, js_init_fs, (void), {
	FS.mount(IDBFS, {}, "/home/web_user");
	await new Promise((resolve, reject) => {
		FS.syncfs(true, (err) => {
			if (err) { reject(err); } else { resolve(); }
		});
	});
})

void
init_fs(void) {
	js_init_fs();
}

void
sync_fs(void) {
	js_sync_fs();
}

#else

void
init_fs(void) {
}

void
sync_fs(void) {
}

#endif
