/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2015 University of Antwerp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "d7ap_stack.h"
#include "shell.h"
#include "debug.h"
#include "framework_defs.h"

void d7ap_stack_init(fs_init_args_t* fs_init_args, d7asp_init_args_t* d7asp_init_args, bool enable_shell, alp_cmd_handler_appl_itf_callback alp_cmd_handler_appl_itf_cb)
{
    assert(fs_init_args != NULL);
    assert(fs_init_args->access_profiles_count > 0); // there should be at least one access profile defined

    fs_init(fs_init_args);
    d7asp_init(d7asp_init_args);
    d7atp_init();
    d7anp_init();
    packet_queue_init();
    dll_init();

    uint8_t read_firmware_version_alp_command[] = { 0x01, D7A_FILE_FIRMWARE_VERSION_FILE_ID, 0, D7A_FILE_FIRMWARE_VERSION_SIZE };

    if(enable_shell)
    {
#ifdef FRAMEWORK_SHELL_ENABLED
        shell_init();
        shell_register_handler((cmd_handler_registration_t){ .id = ALP_CMD_HANDLER_ID, .cmd_handler_callback = &alp_cmd_handler });
        alp_cmd_handler_set_appl_itf_callback(alp_cmd_handler_appl_itf_cb);

        // notify booted to serial
        uint8_t alp_response[20];
        uint8_t alp_response_len = 0;
        alp_process_command(read_firmware_version_alp_command, sizeof(read_firmware_version_alp_command), alp_response, &alp_response_len);
        alp_cmd_handler_output_alp_command(alp_response, alp_response_len);
#endif
    }
    else
    {
#ifdef MODULE_D7AP_BROADCAST_VERSION_ON_BOOT_ENABLED
      // notify booted by broadcasting and retrying 3 times (for diagnostics ie to detect reboots)
      // TODO: default access class
      d7asp_fifo_config_t broadcast_fifo_config = {
          .fifo_ctrl_nls = false,
          .fifo_ctrl_stop_on_error = false,
          .fifo_ctrl_preferred = false,
          .fifo_ctrl_state = SESSION_STATE_PENDING,
          .qos = {
            .qos_ctrl_resp_mode = SESSION_RESP_MODE_ANYCAST,
            .qos_retry_single = 3
          },
          .dormant_timeout = 0,
          .start_id = 0,
          .addressee = {
            .ctrl = {
              .has_id = false,
              .virtual_id = false,
              .access_class = 0,
            },
            .id = 0
          }
      };

      uint8_t alp_response[ALP_PAYLOAD_MAX_SIZE] = { 0 };
      uint8_t alp_response_length = 0;
      assert(alp_process_command_fs_itf(read_firmware_version_alp_command, sizeof(read_firmware_version_alp_command), alp_response, &alp_response_length));

      d7asp_queue_alp_actions(&broadcast_fifo_config, alp_response, alp_response_length);
#endif
    }
}
