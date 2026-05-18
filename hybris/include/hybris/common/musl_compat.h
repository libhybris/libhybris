+#include <unistd.h>
+/* taken from glibc unistd.h and fixes musl */
+#ifndef TEMP_FAILURE_RETRY
+#define TEMP_FAILURE_RETRY(expression) \
+  (__extension__                                                              \
+    ({ long int __result;                                                     \
+       do __result = (long int) (expression);                                 \
+       while (__result == -1L && errno == EINTR);                             \
+       __result; }))
+#endif

