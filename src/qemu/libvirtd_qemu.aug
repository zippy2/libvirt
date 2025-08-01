(* /etc/libvirt/qemu.conf *)

module Libvirtd_qemu =
   autoload xfm

   let eol   = del /[ \t]*\n/ "\n"
   let value_sep   = del /[ \t]*=[ \t]*/  " = "
   let indent = del /[ \t]*/ ""

   let array_sep  = del /,[ \t\n]*/ ", "
   let array_start = del /\[[ \t\n]*/ "[ "
   let array_end = del /\]/ "]"

   let str_val = del /\"/ "\"" . store /[^\"]*/ . del /\"/ "\""
   let bool_val = store /0|1/
   let int_val = store /[0-9]+/
   let str_array_element = [ seq "el" . str_val ] . del /[ \t\n]*/ ""
   let str_array_val = counter "el" . array_start . ( str_array_element . ( array_sep . str_array_element ) * ) ? . array_end

   let str_entry       (kw:string) = [ key kw . value_sep . str_val ]
   let bool_entry      (kw:string) = [ key kw . value_sep . bool_val ]
   let int_entry       (kw:string) = [ key kw . value_sep . int_val ]
   let str_array_entry (kw:string) = [ key kw . value_sep . str_array_val ]

   let unlimited_val =  del /\"/ "\"" . store /unlimited/ . del /\"/ "\""
   let limits_entry (kw:string) = [ key kw . value_sep . unlimited_val ] |  [ key kw . value_sep . int_val ]


   (* Config entry grouped by function - same order as example config *)
   let default_tls_entry = str_entry "default_tls_x509_cert_dir"
                 | bool_entry "default_tls_x509_verify"
                 | str_entry "default_tls_x509_secret_uuid"
                 | str_entry "default_tls_priority"

   let vnc_entry = str_entry "vnc_listen"
                 | bool_entry "vnc_auto_unix_socket"
                 | bool_entry "vnc_tls"
                 | str_entry "vnc_tls_x509_cert_dir"
                 | bool_entry "vnc_tls_x509_verify"
                 | str_entry "vnc_tls_x509_secret_uuid"
                 | str_entry "vnc_tls_priority"
                 | str_entry "vnc_password"
                 | bool_entry "vnc_sasl"
                 | str_entry "vnc_sasl_dir"
                 | bool_entry "vnc_allow_host_audio"

   let spice_entry = str_entry "spice_listen"
                 | bool_entry "spice_tls"
                 | str_entry  "spice_tls_x509_cert_dir"
                 | bool_entry "spice_auto_unix_socket"
                 | str_entry "spice_password"
                 | bool_entry "spice_sasl"
                 | str_entry "spice_sasl_dir"

   let rdp_entry = str_entry "rdp_listen"
                 | str_entry "rdp_tls_x509_cert_dir"
                 | str_entry "rdp_username"
                 | str_entry "rdp_password"

   let chardev_entry = bool_entry "chardev_tls"
                 | str_entry "chardev_tls_x509_cert_dir"
                 | bool_entry "chardev_tls_x509_verify"
                 | str_entry "chardev_tls_x509_secret_uuid"
                 | str_entry "chardev_tls_priority"

   let migrate_entry = str_entry "migrate_tls_x509_cert_dir"
                 | bool_entry "migrate_tls_x509_verify"
                 | str_entry "migrate_tls_x509_secret_uuid"
                 | str_entry "migrate_tls_priority"
                 | bool_entry "migrate_tls_force"

   let backup_entry = str_entry "backup_tls_x509_cert_dir"
                 | bool_entry "backup_tls_x509_verify"
                 | str_entry "backup_tls_x509_secret_uuid"
                 | str_entry "backup_tls_priority"

   (* support for vxhs was removed from qemu and the examples were dopped from *)
   (* qemu.conf but these need to stay *)
   let vxhs_entry = bool_entry "vxhs_tls"
                 | str_entry "vxhs_tls_x509_cert_dir"
                 | str_entry "vxhs_tls_x509_secret_uuid"

   let nbd_entry = bool_entry "nbd_tls"
                 | str_entry "nbd_tls_x509_cert_dir"
                 | str_entry "nbd_tls_x509_secret_uuid"
                 | str_entry "nbd_tls_priority"

   let nogfx_entry = bool_entry "nographics_allow_host_audio"

   let remote_display_entry = int_entry "remote_display_port_min"
                 | int_entry "remote_display_port_max"
                 | int_entry "remote_websocket_port_min"
                 | int_entry "remote_websocket_port_max"

   let security_entry = str_entry "security_driver"
                 | bool_entry "security_default_confined"
                 | bool_entry "security_require_confined"
                 | str_entry "user"
                 | str_entry "group"
                 | bool_entry "dynamic_ownership"
                 | bool_entry "remember_owner"
                 | str_array_entry "cgroup_controllers"
                 | str_array_entry "cgroup_device_acl"
                 | int_entry "seccomp_sandbox"
                 | str_array_entry "namespaces"

   let save_entry = str_entry "save_image_format"
                 | str_entry "dump_image_format"
                 | str_entry "snapshot_image_format"
                 | str_entry "auto_dump_path"
                 | bool_entry "auto_dump_bypass_cache"
                 | bool_entry "auto_start_bypass_cache"
                 | int_entry "auto_start_delay"
                 | str_entry "auto_shutdown_try_save"
                 | str_entry "auto_shutdown_try_shutdown"
                 | str_entry "auto_shutdown_poweroff"
                 | int_entry "auto_shutdown_wait"
                 | bool_entry "auto_shutdown_restore"
                 | bool_entry "auto_save_bypass_cache"

   let process_entry = str_entry "hugetlbfs_mount"
                 | str_entry "bridge_helper"
                 | str_entry "pr_helper"
                 | str_entry "slirp_helper"
                 | str_entry "qemu_rdp"
                 | str_entry "dbus_daemon"
                 | bool_entry "set_process_name"
                 | int_entry "max_processes"
                 | int_entry "max_files"
                 | limits_entry "max_core"
                 | bool_entry "dump_guest_core"
                 | str_entry "stdio_handler"
                 | int_entry "max_threads_per_process"
                 | str_entry "sched_core"

   let device_entry = bool_entry "mac_filter"
                 | bool_entry "relaxed_acs_check"
                 | bool_entry "allow_disk_format_probing"
                 | str_entry "lock_manager"

   let rpc_entry = int_entry "max_queued"
                 | int_entry "keepalive_interval"
                 | int_entry "keepalive_count"

   let network_entry = str_entry "migration_address"
                 | int_entry "migration_port_min"
                 | int_entry "migration_port_max"
                 | str_entry "migration_host"

   let log_entry = bool_entry "log_timestamp"

   let nvram_entry = str_array_entry "nvram"

   let debug_level_entry = int_entry "gluster_debug_level"
                 | bool_entry "virtiofsd_debug"
                 | str_entry "deprecation_behavior"

   let memory_entry = str_entry "memory_backing_dir"

   let swtpm_entry = str_entry "swtpm_user"
                | str_entry "swtpm_group"

   let capability_filters_entry = str_array_entry "capability_filters"

   let storage_entry = bool_entry "storage_use_nbdkit"

   let filesystem_entry = str_array_entry "shared_filesystems"

   let default_cpu_deprecated_features = str_entry "default_cpu_deprecated_features"

   (* Entries that used to exist in the config which are now
    * deleted. We keep on parsing them so we don't break
    * ability to parse old configs after upgrade
    *)
   let obsolete_entry = bool_entry "clear_emulator_capabilities"

   (* Each entry in the config is one of the following ... *)
   let entry = default_tls_entry
             | vnc_entry
             | spice_entry
             | rdp_entry
             | chardev_entry
             | migrate_entry
             | backup_entry
             | nogfx_entry
             | remote_display_entry
             | security_entry
             | save_entry
             | process_entry
             | device_entry
             | rpc_entry
             | network_entry
             | log_entry
             | nvram_entry
             | debug_level_entry
             | memory_entry
             | vxhs_entry
             | nbd_entry
             | swtpm_entry
             | capability_filters_entry
             | storage_entry
             | filesystem_entry
             | default_cpu_deprecated_features
             | obsolete_entry

   let comment = [ label "#comment" . del /#[ \t]*/ "# " .  store /([^ \t\n][^\n]*)?/ . del /\n/ "\n" ]
   let empty = [ label "#empty" . eol ]

   let record = indent . entry . eol

   let lns = ( record | comment | empty ) *

   let filter = incl "/etc/libvirt/qemu.conf"
              . Util.stdexcl

   let xfm = transform lns filter
