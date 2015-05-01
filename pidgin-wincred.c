#define PURPLE_PLUGINS

#include <plugin.h>
#include <version.h>

#include <account.h>
#include <signal.h>
#include <core.h>
#include <debug.h>

#include <glib.h>
#include <string.h>

#include <windows.h>
#include <wincred.h>

#define MAX_READ_LEN 2048
#define PLUGIN_ID "core-wincred"


/* function prototypes */
static void keyring_password_store(PurpleAccount *account);
static BOOL keyring_password_get(PurpleAccount *account);
static gchar * create_account_str(PurpleAccount *account);
static void sign_in_cb(PurpleAccount *account, gpointer data);
static void connecting_cb(PurpleAccount *account, gpointer data);
static PurplePluginPrefFrame * get_pref_frame(PurplePlugin *plugin);

/* function definitions */

/* function called when the plugin starts */
static gboolean plugin_load(PurplePlugin *plugin) {

    GList *accountsList;
    void *accountshandle = purple_accounts_get_handle();
    /* notFound will be a list of accounts not found
     * in the keyring */
    GList *notFound = NULL;
    GList *notFound_iter;
    /* The first thing to do is set all the passwords. */
    for (accountsList = purple_accounts_get_all();
         accountsList != NULL;
         accountsList = accountsList->next) {
        PurpleAccount *account = (PurpleAccount *)accountsList->data;
        /* attempt to get and set the password from the keyring */
        BOOL result = keyring_password_get((PurpleAccount *)accountsList->data);
        if (!result) {
            /* add to the list of accounts not found in the keyring */
            notFound = g_list_append(notFound, account);
        }
    }
    /* for the acccounts which were not found in the keyring */
    for (notFound_iter = g_list_first(notFound);
         notFound_iter != NULL;
         notFound_iter = notFound_iter->next) {
        PurpleAccount *account = (PurpleAccount *)notFound_iter->data;
        /* if the password was saved by libpurple before then
         * save it in the keyring, and tell libpurple to forget it */
        if (purple_account_get_remember_password(account)) {
            keyring_password_store(account);
            purple_account_set_remember_password(account, FALSE);
        }
    }
    /* done with the notFound, so free it */
    g_list_free(notFound);
    /* create a signal which monitors whenever an account signs in,
     * so that the callback function can store/update the password */
    purple_signal_connect(accountshandle, "account-signed-on", plugin,
            PURPLE_CALLBACK(sign_in_cb), NULL);
    /* create a signal which monitors whenever an account tries to connect
     * so that the callback can make sure the password is set in pidgin */
    purple_signal_connect(accountshandle, "account-connecting", plugin,
            PURPLE_CALLBACK(connecting_cb), NULL);
    /* at this point, the plugin is set up */
    return TRUE;
}


/* callback to whenever an account is signed in */
static void sign_in_cb(PurpleAccount *account, gpointer data) {
    keyring_password_store(account);
    return;
}

static void memory_clearing_function(PurpleAccount *account) {
    gboolean clear_memory = purple_prefs_get_bool(
            "/plugins/core/wincred/clear_memory");
    if (clear_memory) {
        if (account->password != NULL) {
            g_free(account->password);
            account->password = NULL;
        }
    }
}

/* callback to whenever a function tries to connect
 * this needs to ensure that there is a password
 * this may happen if the password was disabled, then later re-enabled */
static void connecting_cb(PurpleAccount *account, gpointer data) {
    if (account->password == NULL) {
        keyring_password_get(account);
    }
}

/* Creats a newly allocated account string to name the windows
 * credential. The string returned must later be freed with g_free()
 */
static gchar * create_account_str(PurpleAccount *account) {
    gchar *account_str = g_strdup_printf("libpurple/%s/%s",
        account->protocol_id, account->username);
    return (account_str);
}

/* store a password in the keyring */
static void keyring_password_store(PurpleAccount *account) {
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

    /* Clear password from pidgin memory */
    memory_clearing_function(account);
}

/* retrive a password from the keyring */
static BOOL keyring_password_get(PurpleAccount *account) {
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
        /* set the account to not remember passwords */
        purple_account_set_remember_password(account, FALSE);
        /* temporarily set a fake password, then the real one */
        purple_account_set_password(account, "fakedoopdeedoop");
        purple_account_set_password(account, password);
        g_free(password);
    }
    g_free(account_str);
    g_free(unicode_account_str);
    CredFree(cred);
    return result;
}



static gboolean plugin_unload(PurplePlugin *plugin) {
    /* disconnect from signals */
    void *accounts_handle = purple_accounts_get_handle();
    purple_signal_disconnect(accounts_handle, "account-signed-on",
                             plugin, NULL);
    purple_signal_disconnect(accounts_handle, "account-connecting",
                             plugin, NULL);
    return TRUE;
}

static PurplePluginUiInfo prefs_info = {
    get_pref_frame, 0, NULL, NULL, NULL, NULL, NULL
};


static PurplePluginPrefFrame * get_pref_frame(PurplePlugin *plugin) {
    PurplePluginPrefFrame *frame = purple_plugin_pref_frame_new();
    gchar *label = g_strdup_printf("Clear plaintext passwords from memory");
    PurplePluginPref *pref = purple_plugin_pref_new_with_name_and_label(
            "/plugins/core/wincred/clear_memory",
            label);
    purple_plugin_pref_frame_add(frame, pref);
    return frame;
}


static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC, PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_HIGHEST,

    PLUGIN_ID,
    "Windows Credentials",
    /* version */
    "0.5",

    "Save passwords as windows credentials instead of as plaintext",
    "Save passwords as windows credentials instead of as plaintext",
    "Ali Ebrahim <ali.ebrahim314@gmail.com>",
    "https://github.com/aebrahim/pidgin-wincred",

    plugin_load,
    plugin_unload,
    NULL,
    NULL,
    NULL,
    &prefs_info,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static void init_plugin(PurplePlugin *plugin) {
    purple_prefs_add_none("/plugins/core/wincred");
    purple_prefs_add_bool("/plugins/core/wincred/clear_memory", FALSE);
}

PURPLE_INIT_PLUGIN(wincred, init_plugin, info)
