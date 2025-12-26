#ifndef KAFD_UTILS_H
#define KAFD_UTILS_H

bool ensure_dir(const char *path);
bool ensureSSHKey(void);
bool testSSHAccess(const char *user, const char *host);
bool ensureRemoteAuth(const char *user, const char *host);

#endif
