
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <libpq-fe.h>

int l_PQsetdbLogin(lua_State *L)
{
    const char *pghost = lua_tostring(L, 1);
    const char *pgport = lua_tostring(L, 2);
    const char *dbName = lua_tostring(L, 3);
    const char *login = lua_tostring(L, 4);
    const char *pwd = lua_tostring(L, 5);

    PGconn *conn = PQsetdbLogin(pghost, pgport, NULL, NULL, dbName, login, pwd);

    lua_pushlightuserdata(L, conn);

    return 1;
}

int l_PQconnectdb(lua_State *L)
{
    const char *conninfo = lua_tostring(L, 1);

    PGconn *conn = PQconnectdb(conninfo);

    lua_pushlightuserdata(L, conn);

    return 1;
}

int l_PQconninfo(lua_State *L)
{
    PGconn *conn = (PGconn *)lua_touserdata(L, 1);

    PQconninfoOption *info = PQconninfo(conn);

    lua_newtable(L);

    lua_pushstring(L, info->keyword);
    lua_setfield(L, -2, "keyword");

    lua_pushstring(L, info->val);
    lua_setfield(L, -2, "val");

    lua_pushstring(L, info->label);
    lua_setfield(L, -2, "label");

    lua_pushstring(L, info->dispchar);
    lua_setfield(L, -2, "dispchar");

    PQconninfoFree(info);

    return 1;
}

int l_PQfinish(lua_State *L)
{
    PGconn *conn = (PGconn *)lua_touserdata(L, 1);

    PQfinish(conn);

    return 0;
}

int l_PQstatus(lua_State *L)
{
    PGconn *conn = (PGconn *)lua_touserdata(L, 1);

    ConnStatusType status = PQstatus(conn);

    lua_pushinteger(L, status);

    return 1;
}

int l_PQserverVersion(lua_State *L)
{
    PGconn *conn = (PGconn *)lua_touserdata(L, 1);

    int v = PQserverVersion(conn);

    lua_pushinteger(L, v);

    return 1;
}

int l_PQerrorMessage(lua_State *L)
{
    PGconn *conn = (PGconn *)lua_touserdata(L, 1);

    char *v = PQerrorMessage(conn);

    lua_pushstring(L, v);

    return 1;
}

int l_PQexec(lua_State *L)
{
    PGconn *conn = (PGconn *)lua_touserdata(L, 1);
    const char *command = lua_tostring(L, 2);

    PGresult *res = PQexec(conn, command);

    lua_pushlightuserdata(L, res);

    return 1;
}

int l_PQresultErrorMessage(lua_State *L)
{
    PGresult *res = (PGresult *)lua_touserdata(L, 1);

    lua_pushstring(L, PQresultErrorMessage(res));

    return 1;
}

int l_PQresultStatus(lua_State *L)
{
    PGresult *res = (PGresult *)lua_touserdata(L, 1);

    lua_pushinteger(L, PQresultStatus(res));

    return 1;
}

int l_PQcmdStatus(lua_State *L)
{
    PGresult *res = (PGresult *)lua_touserdata(L, 1);

    lua_pushstring(L, PQcmdStatus(res));

    return 1;
}

int l_PQcmdTuples(lua_State *L)
{
    PGresult *res = (PGresult *)lua_touserdata(L, 1);

    lua_pushstring(L, PQcmdTuples(res));

    return 1;
}

int l_PQprint(lua_State *L)
{

    PGresult *res = (PGresult *)lua_touserdata(L, 1);

    PQprintOpt opt;

    opt.header = lua_toboolean(L, 2);
    opt.align = lua_toboolean(L, 3);
    opt.standard = lua_toboolean(L, 4);
    opt.html3 = lua_toboolean(L, 5);
    opt.expanded = lua_toboolean(L, 6);
    opt.pager = lua_toboolean(L, 7);
    opt.fieldSep = lua_tostring(L, 8);
    opt.tableOpt = lua_tostring(L, 9);
    opt.caption = lua_tostring(L, 10);
    opt.fieldName = NULL;

    FILE *fout = tmpfile();

    PQprint(fout, res, &opt);

    rewind(fout);

    luaL_Buffer b;

    luaL_buffinit(L, &b);

    int c;
    while ((c = fgetc(fout)) != EOF)
        luaL_addchar(&b, c);

    luaL_pushresult(&b);

    fclose(fout);

    return 1;
}

int l_tracing(lua_State *L)
{

    PGconn *conn = (PGconn *)lua_touserdata(L, 1);

    int flags = (lua_toboolean(L, 2) ? PQTRACE_SUPPRESS_TIMESTAMPS : 0) |
                (lua_toboolean(L, 3) ? PQTRACE_REGRESS_MODE : 0);

    FILE *fout = tmpfile();

    PQtrace(conn, fout);

    PQsetTraceFlags(conn, flags);

    int ret = lua_pcall(L, 0, LUA_MULTRET, 0);

    PQuntrace(conn);

    lua_pushboolean(L, ret == LUA_OK);

    rewind(fout);

    luaL_Buffer b;

    luaL_buffinit(L, &b);

    int c;
    while ((c = fgetc(fout)) != EOF)
        luaL_addchar(&b, c);

    luaL_pushresult(&b);

    fclose(fout);

    return 2;
}

int l_PQclear(lua_State *L)
{

    PGresult *res = (PGresult *)lua_touserdata(L, 1);

    PQclear(res);

    return 0;
}

int l_tuples(lua_State *L)
{

    PGresult *res = (PGresult *)lua_touserdata(L, 1);
    int usenames = lua_toboolean(L, 2);

    int ntuples = PQntuples(res);
    int nfields = PQnfields(res);
    int R = -1;
    int C = -1;
    char *v = NULL;

    if (lua_isinteger(L, 3))
    {
        R = lua_tointeger(L, 3);
        ntuples = ntuples < R ? ntuples : R;
    }
    else
        R = 1;

    if (lua_isstring(L, 4))
    {
        v = lua_tostring(L, 4);
        C = PQfnumber(res, v) + 1;

        lua_pop(L, 1);
        lua_pushinteger(L, C);
    }

    if (lua_isinteger(L, 4))
    {
        C = lua_tointeger(L, 4);
        nfields = nfields < C ? nfields : C;
    }
    else
        C = 1;

    lua_newtable(L);

    for (int r = R - 1; r < ntuples; r++)
    {
        lua_newtable(L);

        for (int c = C - 1; c < nfields; c++)
        {
            if (PQgetisnull(res, r, c))
            {
                lua_pushnil(L);
            }
            else
            {
                v = PQgetvalue(res, r, c);
                lua_pushstring(L, v);
            }

            usenames ? lua_setfield(L, -2, PQfname(res, c)) : lua_seti(L, -2, c + 1);
        }

        lua_seti(L, -2, r + 1);
    }

    return 1;
}

void enum_CONNECTION(lua_State *L)
{
    lua_newtable(L);

    lua_pushinteger(L, CONNECTION_OK);
    lua_setfield(L, -2, "OK");

    lua_pushinteger(L, CONNECTION_BAD);
    lua_setfield(L, -2, "BAD");

    lua_pushinteger(L, CONNECTION_STARTED);
    lua_setfield(L, -2, "STARTED");

    lua_pushinteger(L, CONNECTION_MADE);
    lua_setfield(L, -2, "MADE");

    lua_pushinteger(L, CONNECTION_AWAITING_RESPONSE);
    lua_setfield(L, -2, "AWAITING_RESPONSE");

    lua_pushinteger(L, CONNECTION_AUTH_OK);
    lua_setfield(L, -2, "AUTH_OK");

    lua_pushinteger(L, CONNECTION_SETENV);
    lua_setfield(L, -2, "SETENV");

    lua_pushinteger(L, CONNECTION_SSL_STARTUP);
    lua_setfield(L, -2, "SSL_STARTUP");

    lua_pushinteger(L, CONNECTION_NEEDED);
    lua_setfield(L, -2, "NEEDED");

    lua_pushinteger(L, CONNECTION_CHECK_WRITABLE);
    lua_setfield(L, -2, "CHECK_WRITABLE");

    lua_pushinteger(L, CONNECTION_CONSUME);
    lua_setfield(L, -2, "CONSUME");

    lua_pushinteger(L, CONNECTION_GSS_STARTUP);
    lua_setfield(L, -2, "GSS_STARTUP");

    lua_pushinteger(L, CONNECTION_CHECK_TARGET);
    lua_setfield(L, -2, "CHECK_TARGET");

    lua_pushinteger(L, CONNECTION_CHECK_STANDBY);
    lua_setfield(L, -2, "CHECK_STANDBY");

    lua_setfield(L, -2, "CONNECTION");
}

void enum_PGRES(lua_State *L)
{
    lua_newtable(L);

    lua_pushinteger(L, PGRES_EMPTY_QUERY);
    lua_setfield(L, -2, "EMPTY_QUERY");

    lua_pushinteger(L, PGRES_COMMAND_OK);
    lua_setfield(L, -2, "COMMAND_OK");

    lua_pushinteger(L, PGRES_TUPLES_OK);
    lua_setfield(L, -2, "TUPLES_OK");

    lua_pushinteger(L, PGRES_COPY_OUT);
    lua_setfield(L, -2, "COPY_OUT");

    lua_pushinteger(L, PGRES_COPY_IN);
    lua_setfield(L, -2, "COPY_IN");

    lua_pushinteger(L, PGRES_BAD_RESPONSE);
    lua_setfield(L, -2, "BAD_RESPONSE");

    lua_pushinteger(L, PGRES_NONFATAL_ERROR);
    lua_setfield(L, -2, "NONFATAL_ERROR");

    lua_pushinteger(L, PGRES_FATAL_ERROR);
    lua_setfield(L, -2, "FATAL_ERROR");

    lua_pushinteger(L, PGRES_COPY_BOTH);
    lua_setfield(L, -2, "COPY_BOTH");

    lua_pushinteger(L, PGRES_SINGLE_TUPLE);
    lua_setfield(L, -2, "SINGLE_TUPLE");

    lua_pushinteger(L, PGRES_PIPELINE_SYNC);
    lua_setfield(L, -2, "PIPELINE_SYNC");

    lua_pushinteger(L, PGRES_PIPELINE_ABORTED);
    lua_setfield(L, -2, "PIPELINE_ABORTED");

    lua_setfield(L, -2, "PGRES");
}

const struct luaL_Reg libpgsqllua[] = {
    {"setdbLogin", l_PQsetdbLogin},
    {"connectdb", l_PQconnectdb},
    {"conninfo", l_PQconninfo},
    {"finish", l_PQfinish},
    {"status", l_PQstatus},
    {"serverVersion", l_PQserverVersion},
    {"exec", l_PQexec},
    {"print", l_PQprint},
    {"clear", l_PQclear},
    {"errorMessage", l_PQerrorMessage},
    {"resultErrorMessage", l_PQresultErrorMessage},
    {"resultStatus", l_PQresultStatus},
    {"cmdStatus", l_PQcmdStatus},
    {"cmdTuples", l_PQcmdTuples},
    {"tuples", l_tuples},
    {"tracing", l_tracing},
    {NULL, NULL} /* sentinel */
};

extern int luaopen_libpgsqllua(lua_State *L)
{
    luaL_newlib(L, libpgsqllua);

    enum_PGRES(L);
    enum_CONNECTION(L);

    return 1;
}
