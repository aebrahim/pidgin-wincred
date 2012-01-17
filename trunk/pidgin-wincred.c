#define PURPLE_PLUGINS

#include <plugin.h>
#include <version.h>

#include <account.h>
#include <signal.h>
#include <core.h>

#include <glib.h>
#include <string.h>

#include <windows.h>
#include <wincred.h>

/* function prototypes */
static void keyring_password_store(PurpleAccount *account, char *password);
static BOOL keyring_password_get(PurpleAccount *account);
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
            gchar *password = g_strdup(account->password);
            keyring_password_store(account, password);
            purple_account_set_remember_password(account, FALSE);
            /* temporarily set a fake password, then the real one again */
            purple_account_set_password(account, "fakedoopdeedoop");
            purple_account_set_password(account, password);
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
    keyring_password_store(account, account->password);
    return;
}

/* callback to whenever a function tries to connect
 * this needs to ensure that there is a password
 * this may happen if the password was disabled, then later re-enabled */
static void connecting_cb(PurpleAccount *account, gpointer data) {
	if (account->password == NULL) {
		keyring_password_get(account);
	}
}

/* store a password in the keyring */
static void keyring_password_store(PurpleAccount *account,
                                   char *password) {
	int length = strlen(password);
	WCHAR *wpass = (WCHAR *)malloc(length * sizeof(WCHAR));
	char *account_str = g_strdup_printf("libpurple/%s/%s",
			account->protocol_id, account->username);
	CREDENTIAL cred = {0};
	/* the password will need to be saved as WCHAR */
	swprintf(wpass, L"%S", password);
	cred.Type = CRED_TYPE_GENERIC;
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	cred.TargetName = account_str;
	cred.UserName = account_str;
    cred.CredentialBlob = (BYTE *)wpass;
	cred.CredentialBlobSize = sizeof(BYTE) * sizeof(WCHAR) * length;
	CredWrite(&cred, 0);
	free(wpass);
	g_free(account_str);
}

/* retrive a password from the keyring */
static BOOL keyring_password_get(PurpleAccount *account) {
	PCREDENTIALA cred;
	char *account_str = g_strdup_printf("libpurple/%s/%s",
		account->protocol_id, account->username);
	BOOL result = CredReadA(account_str, CRED_TYPE_GENERIC, 0, &cred);
	/* if the password exists in the keyring, set it in pidgin */
	if (result) {
		gchar *password = g_strdup_printf("%S", (WCHAR *)cred->CredentialBlob);
		purple_account_set_password(account, password);
		/* set the account to not remember passwords */
		purple_account_set_remember_password(account, FALSE);
		/* temporarily set a fake password, then the real one */
		purple_account_set_password(account, "fakedoopdeedoop");
		password = g_strdup_printf("%S", (WCHAR *)cred->CredentialBlob);
		purple_account_set_password(account, password);
		g_free(password);
	}
	g_free(account_str);
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
    /*gchar *label = g_strdup_printf("Should passwords be wiped from pidgin's"
               " memory?\nNote: enabling this setting might break things,\n"
               "as some functions might need the password to be in memory.");
    PurplePluginPref *pref = purple_plugin_pref_new_with_name_and_label(
            "/plugins/core/wincred/clear_memory",
            label);
    purple_plugin_pref_frame_add(frame, pref);*/
    return frame;
}


static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC, PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_HIGHEST,
    
    "core-wincred",
    "Windows Credentials",
    /* version */
    "0.1",

    "Save passwords as windows credentials instead of as plaintext",
    "Save passwords as windows credentials instead of as plaintext",
    "Ali Ebrahim",
    "http://code.google.com/p/pidgin-wincred/",     
    
    plugin_load,                   
    plugin_unload,                          
    NULL,                          
    NULL,                          
    NULL,                          
    NULL, //&prefs_info,                        
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};                               
    
static void init_plugin(PurplePlugin *plugin) {                       
    purple_prefs_add_none("/plugins/core/wincred");
    //purple_prefs_add_bool("/plugins/core/wincred/clear_memory", FALSE);
}

PURPLE_INIT_PLUGIN(wincred, init_plugin, info)