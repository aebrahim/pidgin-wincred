#include "keyring_plugin.h"
#include "keyring_plugin_defs.h"

#include <debug.h>

#include <glib.h>
#include <string.h>

#include <windows.h>
#include <wincred.h>

#define MAX_READ_LEN 2048


/* function prototypes */
static gchar * create_account_str(PurpleAccount *account);

/* function definitions */

/* Creats a newly allocated account string to name the windows
 * credential. The string returned must later be freed with g_free()
 */
static gchar * create_account_str(PurpleAccount *account) {
    gchar *account_str = g_strdup_printf("libpurple/%s/%s",
        account->protocol_id, account->username);
    return (account_str);
}

/* store a password in the keyring */
void keyring_password_store(PurpleAccount *account) {
    char *password;
    int length;
    gchar *account_str;
    gunichar2 *unicode_password;
    gunichar2 *unicode_account_str;
    CREDENTIALW cred = {0};

    password = account->password;
    /* can't store the password if it doesn't exist. Also, str operations
     * on NULL will crash */
    if (password == NULL) return;
    length = strlen(password);
    account_str = create_account_str(account);
    /* unicode encodings of the password and account string */
    unicode_password = g_utf8_to_utf16(password, length, NULL, NULL, NULL);
    unicode_account_str = g_utf8_to_utf16(account_str, strlen(account_str),
        NULL, NULL, NULL);
    /* make the credential */
    cred.Type = CRED_TYPE_GENERIC;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.TargetName = unicode_account_str;
    cred.UserName = unicode_account_str;
    cred.CredentialBlob = (BYTE *) unicode_password;
    cred.CredentialBlobSize = sizeof(BYTE) * sizeof(gunichar2) * length;
    /* write the credential, then free memory */
    CredWriteW(&cred, 0);
    g_free(account_str);
    g_free(unicode_password);
    g_free(unicode_account_str);

}

/* retrive a password from the keyring */
void keyring_password_get(PurpleAccount *account) {
    gchar *account_str = create_account_str(account);
    gunichar2 *unicode_account_str = g_utf8_to_utf16(account_str,
        strlen(account_str), NULL, NULL, NULL);
    PCREDENTIALW cred;
    BOOL result = CredReadW(unicode_account_str, CRED_TYPE_GENERIC, 0, &cred);
    /* if the password exists in the keyring, set it in pidgin */
    if (result) {
        /* decode the password from unicode */
        gchar *password = g_utf16_to_utf8((gunichar2 *)cred->CredentialBlob,
            MAX_READ_LEN, NULL, NULL, NULL);
        /* process the password in case account_str was appended to
         * the end, as observed in some cases */
        char *appended = strstr(password, account_str);
        if (appended != NULL) {
            int actual_length = appended - password;
            gchar *fixed_password = g_strndup(password, actual_length);
            g_free(password);
            password = fixed_password;
            purple_debug_warning(PLUGIN_ID,
                "account_string %s detected at the end of the password\n",
                account_str);
        }
        purple_account_set_password(account, password);
        g_free(password);
    }
    g_free(account_str);
    g_free(unicode_account_str);
    CredFree(cred);
}

PurplePluginPrefFrame * get_pref_frame(PurplePlugin *plugin) {
    PurplePluginPrefFrame *frame = purple_plugin_pref_frame_new();
    gchar *label = g_strdup_printf("Clear plaintext passwords from memory");
    PurplePluginPref *pref = purple_plugin_pref_new_with_name_and_label(
            "/plugins/core/wincred/clear_memory",
            label);
    purple_plugin_pref_frame_add(frame, pref);
    return frame;
}


void init_keyring_plugin(PurplePlugin *plugin) {
    purple_prefs_add_none("/plugins/core/wincred");
}
