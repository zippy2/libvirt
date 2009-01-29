module Test_libvirtd_qemu =

   let conf = "# Master configuration file for the QEMU driver.
# All settings described here are optional - if omitted, sensible
# defaults are used.

# VNC is configured to listen on 127.0.0.1 by default.
# To make it listen on all public interfaces, uncomment
# this next option.
#
# NB, strong recommendation to enable TLS + x509 certificate
# verification when allowing public access
#
vnc_listen = \"0.0.0.0\"


# Enable use of TLS encryption on the VNC server. This requires
# a VNC client which supports the VeNCrypt protocol extension.
# Examples include vinagre, virt-viewer, virt-manager and vencrypt
# itself. UltraVNC, RealVNC, TightVNC do not support this
#
# It is necessary to setup CA and issue a server certificate
# before enabling this.
#
vnc_tls = 1


# Use of TLS requires that x509 certificates be issued. The
# default it to keep them in /etc/pki/libvirt-vnc. This directory
# must contain
#
#  ca-cert.pem - the CA master certificate
#  server-cert.pem - the server certificate signed with ca-cert.pem
#  server-key.pem  - the server private key
#
# This option allows the certificate directory to be changed
#
vnc_tls_x509_cert_dir = \"/etc/pki/libvirt-vnc\"


# The default TLS configuration only uses certificates for the server
# allowing the client to verify the server's identity and establish
# and encrypted channel.
#
# It is possible to use x509 certificates for authentication too, by
# issuing a x509 certificate to every client who needs to connect.
#
# Enabling this option will reject any client who does not have a
# certificate signed by the CA in /etc/pki/libvirt-vnc/ca-cert.pem
#
vnc_tls_x509_verify = 1


# The default VNC password. Only 8 letters are significant for
# VNC passwords. This parameter is only used if the per-domain
# XML config does not already provide a password. To allow
# access without passwords, leave this commented out. An empty
# string will still enable passwords, but be rejected by QEMU
# effectively preventing any use of VNC. Obviously change this
# example here before you set this
#
vnc_password = \"XYZ12345\"
"

   test Libvirtd_qemu.lns get conf =
{ "#comment" = "Master configuration file for the QEMU driver." }
{ "#comment" = "All settings described here are optional - if omitted, sensible" }
{ "#comment" = "defaults are used." }
{ "#empty" }
{ "#comment" = "VNC is configured to listen on 127.0.0.1 by default." }
{ "#comment" = "To make it listen on all public interfaces, uncomment" }
{ "#comment" = "this next option." }
{ "#comment" = "" }
{ "#comment" = "NB, strong recommendation to enable TLS + x509 certificate" }
{ "#comment" = "verification when allowing public access" }
{ "#comment" = "" }
{ "vnc_listen" = "0.0.0.0" }
{ "#empty" }
{ "#empty" }
{ "#comment" = "Enable use of TLS encryption on the VNC server. This requires" }
{ "#comment" = "a VNC client which supports the VeNCrypt protocol extension." }
{ "#comment" = "Examples include vinagre, virt-viewer, virt-manager and vencrypt" }
{ "#comment" = "itself. UltraVNC, RealVNC, TightVNC do not support this" }
{ "#comment" = "" }
{ "#comment" = "It is necessary to setup CA and issue a server certificate" }
{ "#comment" = "before enabling this." }
{ "#comment" = "" }
{ "vnc_tls" = "1" }
{ "#empty" }
{ "#empty" }
{ "#comment" = "Use of TLS requires that x509 certificates be issued. The" }
{ "#comment" = "default it to keep them in /etc/pki/libvirt-vnc. This directory" }
{ "#comment" = "must contain" }
{ "#comment" = "" }
{ "#comment" = "ca-cert.pem - the CA master certificate" }
{ "#comment" = "server-cert.pem - the server certificate signed with ca-cert.pem" }
{ "#comment" = "server-key.pem  - the server private key" }
{ "#comment" = "" }
{ "#comment" = "This option allows the certificate directory to be changed" }
{ "#comment" = "" }
{ "vnc_tls_x509_cert_dir" = "/etc/pki/libvirt-vnc" }
{ "#empty" }
{ "#empty" }
{ "#comment" = "The default TLS configuration only uses certificates for the server" }
{ "#comment" = "allowing the client to verify the server's identity and establish" }
{ "#comment" = "and encrypted channel." }
{ "#comment" = "" }
{ "#comment" = "It is possible to use x509 certificates for authentication too, by" }
{ "#comment" = "issuing a x509 certificate to every client who needs to connect." }
{ "#comment" = "" }
{ "#comment" = "Enabling this option will reject any client who does not have a" }
{ "#comment" = "certificate signed by the CA in /etc/pki/libvirt-vnc/ca-cert.pem" }
{ "#comment" = "" }
{ "vnc_tls_x509_verify" = "1" }
{ "#empty" }
{ "#empty" }
{ "#comment" = "The default VNC password. Only 8 letters are significant for" }
{ "#comment" = "VNC passwords. This parameter is only used if the per-domain" }
{ "#comment" = "XML config does not already provide a password. To allow" }
{ "#comment" = "access without passwords, leave this commented out. An empty" }
{ "#comment" = "string will still enable passwords, but be rejected by QEMU" }
{ "#comment" = "effectively preventing any use of VNC. Obviously change this" }
{ "#comment" = "example here before you set this" }
{ "#comment" = "" }
{ "vnc_password" = "XYZ12345" }
