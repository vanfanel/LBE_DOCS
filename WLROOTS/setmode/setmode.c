#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include "wlr-output-management-unstable-v1-client-protocol.h"

struct randr_state;
struct randr_head;

struct randr_mode {
	struct randr_head *head;
	struct zwlr_output_mode_v1 *wlr_mode;
	struct wl_list link;

	int32_t width, height;
	int32_t refresh; // mHz
	bool preferred;
};

struct randr_head {
	struct randr_state *state;
	struct zwlr_output_head_v1 *wlr_head;
	struct wl_list link;

	char *name, *description;
	char *make, *model, *serial_number;
	int32_t phys_width, phys_height; // mm
	struct wl_list modes;

	uint32_t changed; // enum randr_head_prop
	bool enabled;
	struct randr_mode *mode;
	struct {
		int32_t width, height;
		int32_t refresh;
	} custom_mode;
	int32_t x, y;
	enum wl_output_transform transform;
	double scale;
};

struct randr_state {
	struct zwlr_output_manager_v1 *output_manager;

	struct wl_list heads;
	uint32_t serial;
	bool has_serial;
	bool running;
	bool failed;
};

static void config_handle_succeeded(void *data,
		struct zwlr_output_configuration_v1 *config) {
	struct randr_state *state = data;
	zwlr_output_configuration_v1_destroy(config);
	state->running = false;
}

static void config_handle_failed(void *data,
		struct zwlr_output_configuration_v1 *config) {
	struct randr_state *state = data;
	zwlr_output_configuration_v1_destroy(config);
	state->running = false;
	state->failed = true;

	fprintf(stderr, "failed to apply configuration\n");
}

static void config_handle_cancelled(void *data,
		struct zwlr_output_configuration_v1 *config) {
	struct randr_state *state = data;
	zwlr_output_configuration_v1_destroy(config);
	state->running = false;
	state->failed = true;

	fprintf(stderr, "configuration cancelled, please try again\n");
}

static const struct zwlr_output_configuration_v1_listener config_listener = {
	.succeeded = config_handle_succeeded,
	.failed = config_handle_failed,
	.cancelled = config_handle_cancelled,
};

static void apply_state(struct randr_state *state) {
	struct zwlr_output_configuration_v1 *config =
		zwlr_output_manager_v1_create_configuration(state->output_manager,
		state->serial);
	zwlr_output_configuration_v1_add_listener(config, &config_listener, state);

	struct randr_head *head;
	wl_list_for_each(head, &state->heads, link) {
		if (!head->enabled) {
			zwlr_output_configuration_v1_disable_head(config, head->wlr_head);
			continue;
		}

		struct zwlr_output_configuration_head_v1 *config_head =
			zwlr_output_configuration_v1_enable_head(config, head->wlr_head);

		if (head->mode != NULL) {
			zwlr_output_configuration_head_v1_set_mode(config_head,
				head->mode->wlr_mode);
		} else {
			zwlr_output_configuration_head_v1_set_custom_mode(config_head,
				head->custom_mode.width, head->custom_mode.height,
				head->custom_mode.refresh);
		}

		zwlr_output_configuration_head_v1_destroy(config_head);
	}

	zwlr_output_configuration_v1_apply(config);
}

static void mode_handle_size(void *data, struct zwlr_output_mode_v1 *wlr_mode,
		int32_t width, int32_t height) {
	struct randr_mode *mode = data;
	mode->width = width;
	mode->height = height;
}

static void mode_handle_refresh(void *data,
		struct zwlr_output_mode_v1 *wlr_mode, int32_t refresh) {
	struct randr_mode *mode = data;
	mode->refresh = refresh;
}

static void mode_handle_preferred(void *data,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_mode *mode = data;
	mode->preferred = true;
}

static void mode_handle_finished(void *data,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_mode *mode = data;
	wl_list_remove(&mode->link);
	if (zwlr_output_mode_v1_get_version(mode->wlr_mode) >= 3) {
		zwlr_output_mode_v1_release(mode->wlr_mode);
	} else {
		zwlr_output_mode_v1_destroy(mode->wlr_mode);
	}
	free(mode);
}

static const struct zwlr_output_mode_v1_listener mode_listener = {
	.size = mode_handle_size,
	.refresh = mode_handle_refresh,
	.preferred = mode_handle_preferred,
	.finished = mode_handle_finished,
};

static void head_handle_name(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *name) {
	struct randr_head *head = data;
	head->name = strdup(name);
}

static void head_handle_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_head *head = data;

	struct randr_mode *mode = calloc(1, sizeof(*mode));
	mode->head = head;
	mode->wlr_mode = wlr_mode;
	wl_list_insert(head->modes.prev, &mode->link);

	zwlr_output_mode_v1_add_listener(wlr_mode, &mode_listener, mode);
}

static void head_handle_enabled(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t enabled) {
	struct randr_head *head = data;
	head->enabled = !!enabled;
	if (!enabled) {
		head->mode = NULL;
	}
}

static void head_handle_current_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_head *head = data;
	struct randr_mode *mode;
	wl_list_for_each(mode, &head->modes, link) {
		if (mode->wlr_mode == wlr_mode) {
			head->mode = mode;
			return;
		}
	}
	fprintf(stderr, "received unknown current_mode\n");
	head->mode = NULL;
}

static void head_handle_finished(void *data,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_head *head = data;
	wl_list_remove(&head->link);
	if (zwlr_output_head_v1_get_version(head->wlr_head) >= 3) {
		zwlr_output_head_v1_release(head->wlr_head);
	} else {
		zwlr_output_head_v1_destroy(head->wlr_head);
	}
	free(head->name);
	free(head->description);
	free(head);
}

static const struct zwlr_output_head_v1_listener head_listener = {
	.name = head_handle_name,
	.mode = head_handle_mode,
	.enabled = head_handle_enabled,
	.current_mode = head_handle_current_mode,
	.finished = head_handle_finished
};

static void output_manager_handle_head(void *data,
		struct zwlr_output_manager_v1 *manager,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_state *state = data;

	struct randr_head *head = calloc(1, sizeof(*head));
	head->state = state;
	head->wlr_head = wlr_head;
	head->scale = 1.0;
	wl_list_init(&head->modes);
	wl_list_insert(state->heads.prev, &head->link);

	zwlr_output_head_v1_add_listener(wlr_head, &head_listener, head);
}

static void output_manager_handle_done(void *data,
		struct zwlr_output_manager_v1 *manager, uint32_t serial) {
	struct randr_state *state = data;
	state->serial = serial;
	state->has_serial = true;
}

static void output_manager_handle_finished(void *data,
		struct zwlr_output_manager_v1 *manager) {
	// This space is intentionally left blank
}

static const struct zwlr_output_manager_v1_listener output_manager_listener = {
	.head = output_manager_handle_head,
	.done = output_manager_handle_done,
	.finished = output_manager_handle_finished,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct randr_state *state = data;

	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		uint32_t version_to_bind = version <= 4 ? version : 4;
		state->output_manager = wl_registry_bind(registry, name,
			&zwlr_output_manager_v1_interface, version_to_bind);
		zwlr_output_manager_v1_add_listener(state->output_manager,
			&output_manager_listener, state);
	}
}

static void registry_handle_global_remove(void *data,
		struct wl_registry *registry, uint32_t name) {
	// This space is intentionally left blank
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

int WL_SetMode(const char *connector_name, int width, int height, int refresh /* In MHz, ie: 3 decimals. */) {

	bool connector_found = false;
	struct randr_head *current_head = NULL;
	
	struct randr_state state = { .running = true };
	wl_list_init(&state.heads);

	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to connect to display\n");
		return EXIT_FAILURE;
	}

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, &state);

	if (wl_display_roundtrip(display) < 0) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		return EXIT_FAILURE;
	}

	if (state.output_manager == NULL) {
		fprintf(stderr, "compositor doesn't support "
			"wlr-output-management-unstable-v1\n");
		return EXIT_FAILURE;
	}

	while (!state.has_serial) {
		if (wl_display_dispatch(display) < 0) {
			fprintf(stderr, "wl_display_dispatch failed\n");
			return EXIT_FAILURE;
		}
	}

	/////// List monitors = connectors
	/*wl_list_for_each(current_head, &state.heads, link) {
		printf ("Connector: %s\n", current_head->name);
		printf("Make: %s\n", current_head->make);
		printf("Model: %s\n", current_head->model);
		printf("Enabled: %s\n\n",current_head->enabled ? "yes" : "no");
	}*/
	///////
	
	wl_list_for_each(current_head, &state.heads, link) {
		if (strcmp(current_head->name, connector_name) == 0) {
			connector_found = true;
			break;
		}
	}
	if (!connector_found) {
		fprintf(stderr, "unknown output %s\n", connector_name);
		return EXIT_FAILURE;
	}

	// Fill-in the new mode we want
	current_head->mode = NULL;
	current_head->custom_mode.width = width;
	current_head->custom_mode.height = height;
	current_head->custom_mode.refresh = refresh;

	apply_state(&state);

	while (state.running && wl_display_dispatch(display) != -1) {
		// This space intentionally left blank
	}

	struct randr_head *head, *tmp_head;
	wl_list_for_each_safe(head, tmp_head, &state.heads, link) {
		struct randr_mode *mode, *tmp_mode;
		wl_list_for_each_safe(mode, tmp_mode, &head->modes, link) {
			zwlr_output_mode_v1_destroy(mode->wlr_mode);
			free(mode);
		}
		zwlr_output_head_v1_destroy(head->wlr_head);
		free(head->name);
		free(head->description);
		free(head->make);
		free(head->model);
		free(head->serial_number);
		free(head);
	}
	zwlr_output_manager_v1_destroy(state.output_manager);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);

	return state.failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main () {

	const char connector_name[] = "HDMI-A-1";

	int width = 1920;
	int height = 1080;
	int refresh = 55213;

	WL_SetMode(connector_name, width, height, refresh);
}
