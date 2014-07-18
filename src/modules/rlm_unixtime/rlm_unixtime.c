/*
 *   This program is is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License, version 2 if the
 *   License as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * $Id$
 * @file rlm_unixtime.c
 * @brief UNIX epoch-based time support
 *
 * @copyright 2013 The FreeRADIUS server project
 * @copyright 2013 Virginia Polytechnic Institute and State University
 */
RCSID("$Id$")

#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/modules.h>
#include <time.h>

/*
 *	Define a structure for our module configuration.
 *
 *	These variables do not need to be in a structure, but it's
 *	a lot cleaner to do so, and a pointer to the structure can
 *	be used as the instance handle.
 */
typedef struct rlm_unixtime_t {
	char const	*list;
 	char const	*attribute;
} rlm_unixtime_t;

/*
 *	A mapping of configuration file names to internal variables.
 */
static const CONF_PARSER module_config[] = {
	{ "list", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_unixtime_t, list), "request" },
	{ "attribute", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_unixtime_t, attribute), "Current-Time" },

	{ NULL, -1, 0, NULL, NULL }		/* end the list */
};


/*
 *	Do any per-module initialization that is separate to each
 *	configured instance of the module.  e.g. set up connections
 *	to external databases, read configuration files, set up
 *	dictionary entries, etc.
 *
 *	If configuration information is given in the config section
 *	that must be referenced in later calls, store a handle to it
 *	in *instance otherwise put a null pointer there.
 */
static int mod_instantiate(UNUSED CONF_SECTION *conf, void *instance)
{
	rlm_unixtime_t *inst = instance;

        if (strcasecmp(inst->list, "request") != 0
	    && strcasecmp(inst->list, "control") != 0
	    && strcasecmp(inst->list, "check") != 0
            && strcasecmp(inst->list, "reply") != 0) {

		cf_log_err_cs(conf, "unrecognized list name");
		return -1;
        }
	return 0;
}

/*
 *	Find the named user in this modules database.  Create the set
 *	of attribute-value pairs to check and reply with for this user
 *	from the database. The authentication code only needs to check
 *	the password, the rest is done here.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_authorize(void *instance, REQUEST *request)
{
	rlm_unixtime_t *inst = instance; 
        char unix_time_str[11];
        time_t unix_time;

        if (time(&unix_time) == -1) {
		return RLM_MODULE_FAIL;
	}

        snprintf(unix_time_str, sizeof(unix_time_str), "%ld", (long) unix_time);

        if (strcasecmp(inst->list, "request") == 0) {
	  pairmake_packet(inst->attribute, unix_time_str, T_OP_EQ);
        }
        else if (strcasecmp(inst->list, "control") == 0
                 || strcasecmp(inst->list, "check") == 0) {
	  pairmake_config(inst->attribute, unix_time_str, T_OP_EQ);
        }
        else if (strcasecmp(inst->list, "reply") == 0) {
	  pairmake_reply(inst->attribute, unix_time_str, T_OP_EQ);
        } 
      
	return RLM_MODULE_OK;
}

/*
 *	Authenticate the user with the given password.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_authenticate(UNUSED void *instance, UNUSED REQUEST *request)
{
	return RLM_MODULE_OK;
}

#ifdef WITH_ACCOUNTING
/*
 *	Massage the request before recording it or proxying it
 */
static rlm_rcode_t CC_HINT(nonnull) mod_preacct(UNUSED void *instance, UNUSED REQUEST *request)
{
	return RLM_MODULE_OK;
}

/*
 *	Write accounting information to this modules database.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_accounting(UNUSED void *instance, UNUSED REQUEST *request)
{
	return RLM_MODULE_OK;
}

/*
 *	See if a user is already logged in. Sets request->simul_count to the
 *	current session count for this user and sets request->simul_mpp to 2
 *	if it looks like a multilink attempt based on the requested IP
 *	address, otherwise leaves request->simul_mpp alone.
 *
 *	Check twice. If on the first pass the user exceeds his
 *	max. number of logins, do a second pass and validate all
 *	logins by querying the terminal server (using eg. SNMP).
 */
static rlm_rcode_t CC_HINT(nonnull) mod_checksimul(UNUSED void *instance, UNUSED REQUEST *request)
{
  request->simul_count=0;

  return RLM_MODULE_OK;
}
#endif


/*
 *	Only free memory we allocated.  The strings allocated via
 *	cf_section_parse() do not need to be freed.
 */
static int mod_detach(UNUSED void *instance)
{
	/* free things here */
	return 0;
}

/*
 *	The module name should be the only globally exported symbol.
 *	That is, everything else should be 'static'.
 *
 *	If the module needs to temporarily modify it's instantiation
 *	data, the type should be changed to RLM_TYPE_THREAD_UNSAFE.
 *	The server will then take care of ensuring that the module
 *	is single-threaded.
 */
module_t rlm_unixtime = {
	RLM_MODULE_INIT,
	"unixtime",
	RLM_TYPE_THREAD_SAFE,		/* type */
	sizeof(rlm_unixtime_t),
	module_config,
	mod_instantiate,		/* instantiation */
	mod_detach,			/* detach */
	{
		mod_authenticate,	/* authentication */
		mod_authorize,	/* authorization */
#ifdef WITH_ACCOUNTING
		mod_preacct,	/* preaccounting */
		mod_accounting,	/* accounting */
		mod_checksimul,	/* checksimul */
#else
		NULL, NULL, NULL,
#endif
		NULL,			/* pre-proxy */
		NULL,			/* post-proxy */
		NULL			/* post-auth */
	},
};
