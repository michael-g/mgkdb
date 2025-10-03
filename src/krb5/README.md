This library implements a shared library which performs Kerberos authentication for kdb+ using GSSAPI. It includes `.q` files to handle both ordinary IPC connections as well as HTTP SPNEGO authentication for browsers. 

For now, please see the accompanying [blog post](https://mindfruit.pages.dev/posts/2025/09/kdb_and_kerberos_authentication/) for a discussion of what's going on here, including instructions on how to set-up a local KDC for testing. 

To get this working, please symlink `../ipcjs/src/ipc.js` to `html/`.


