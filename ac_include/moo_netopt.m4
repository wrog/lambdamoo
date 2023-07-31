# -*- autoconf -*-

# ******************************************************
#  MOO_NET_ENABLE_ARG([net])
#    creates the --enable-net=kwds... ./configure argument
#    sets shell vars for MOO_NET_DO_REPORT:
#      $moo_net_do_{list,check,require}
#    sets shell vars for MOO_OPTION_ARG_ENABLES:
#      $moo_d_{NETWORK_PROTOCOL,NETWORK_STYLE,MPLEX_STYLE,
#              DEFAULT_{PORT,CONNECT_FILE},
#              OUTBOUND_NETWORK}
#    (yes this takes an argument
#    but it is probably a bad idea to change it.)
#
AC_DEFUN([MOO_NET_ARG_ENABLE],
[[if test "x$silent" = xyes ; then
  moo_net_do_list=false
else
  moo_net_do_list=:
fi
moo_net_do_check=:
moo_net_do_require=false

]AC_ARG_ENABLE([$1],
[AS_HELP_STRING([--disable-$1],[NETWORK_PROTOCOL=NP_SINGLE])
AS_HELP_STRING([--enable-$1=KWD,KWD...],
                         [set network options. Allowed KWDs are])
[                  tcp, local:  NETWORK_PROTOCOL=NP_*]
[                   bsd, sysv:  NETWORK_STYLE=NS_*]
[          select, poll, fake:  MPLEX_STYLE=MP_*]
[  path/to/file, '"filename"':  DEFAULT_CONNECT_FILE=*]
[               [port number]:  DEFAULT_PORT=*]
[          noout, outoff, out:  OUTBOUND_NETWORK={undef,-O,+O}]
[                        list:  list available choices]
[             require, nowarn:  if nonviable {error, dont care}]],
[[ac_save_IFS=$IFS
moo_net_do_list=false
IFS=,
for moo_kwd in x $enableval ; do
  IFS=$ac_save_IFS]
  AS_CASE([$moo_kwd],[
    x],     [[continue]],                               [
    list],  [[moo_net_do_list=: ;      continue]],      [
    nowarn],[[moo_net_do_check=false ; continue]],      [
    require],[[moo_net_do_require=: ;  continue]],      [
    out],   [[moo_d=OUTBOUND_NETWORK; moo_v=OBN_ON]],   [
    outoff],[[moo_d=OUTBOUND_NETWORK; moo_v=OBN_OFF]],  [
    noout], [[moo_d=OUTBOUND_NETWORK; moo_v=no]],       [
    no],    [[moo_d=NETWORK_PROTOCOL; moo_v=NP_SINGLE
    	      moo_d_NETWORK_STYLE=no
	      moo_d_MPLEX_STYLE=no
              moo_d_OUTBOUND_NETWORK=no]],[
    tcp],   [[moo_d=NETWORK_PROTOCOL; moo_v=NP_TCP]],   [
    local], [[moo_d=NETWORK_PROTOCOL; moo_v=NP_LOCAL
              moo_d_OUTBOUND_NETWORK=no]], [
    bsd],   [[moo_d=NETWORK_STYLE;    moo_v=NS_BSD]],   [
    sysv],  [[moo_d=NETWORK_STYLE;    moo_v=NS_SYSV]],  [
    select],[[moo_d=MPLEX_STYLE;      moo_v=MP_SELECT]],[
    poll],  [[moo_d=MPLEX_STYLE;      moo_v=MP_POLL]],  [
    fake],  [[moo_d=MPLEX_STYLE;      moo_v=MP_FAKE]],  [
    '"'*'"'], [[moo_d=DEFAULT_CONNECT_FILE; moo_v="$moo_kwd"]],    [
    */*],     [[moo_d=DEFAULT_CONNECT_FILE; moo_v="\"$moo_kwd\""]],[[
    *[!0-9]*]], AC_MSG_ERROR([unknown --enable-$1 keyword: $moo_kwd]),
    [[moo_d=DEFAULT_PORT; moo_v=$moo_kwd]])
  AS_VAR_SET_IF([moo_d_$moo_d],
    [AS_VAR_COPY([moo_v],[moo_d_$moo_d])
     AC_MSG_ERROR([[--enable-$1=...,$moo_kwd: $moo_d already set to $moo_v]])])
  AS_VAR_SET([moo_d_$moo_d],[[$moo_v]])
  AC_MSG_NOTICE([(--enable-$1:) $moo_d = $moo_v])[
done]])])

# ******************************************************
#  MOO_NET_VIABLE
#    build the lists of viable network configurations
#    sets:
#        moo_net_cv_configs = list of ,NP_ROTO/NS_TYLE,
#        moo_net_uconfigs = list of lines to display
#
AC_DEFUN([MOO_NET_VIABLE],
  [AC_REQUIRE([MOO_NET_PREREQUISITES])
AC_REQUIRE([MOO_NET_FIFO_WORKS])
  moo_net_cv_configs=,
  moo_net_uconfigs=
  _MOO_NET_VIABLE_ADD([],[NP_SINGLE])
m4_foreach_w([_moo_np],[[TCP] [LOCAL]],
  [m4_foreach_w([_moo_ns],[[BSD] [SYSV]],
[AS_IF(m4_dquote(m4_indir([_MOO_NET_VIABLE(NP_]_moo_np[/NS_]_moo_ns[)])),
  [_MOO_NET_VIABLE_ADD(m4_tolower(m4_dquote(_moo_ns,_moo_np)),
     m4_dquote([NP_]_moo_np[/NS_]_moo_ns))])
])])])dnl

# _MOO_NET_VIABLE_ADD(KWDLIST, NP_ROTO/NS_TYLE)
# ------------------------
#   add a viable setting to moo_net_cv_{u,}configs
#   KWDLIST = keywords for --enable-net ([] for --disable-net)
#   NP_ROTO/NS_TYLE = corresponding ids for options.h[.in]
#
m4_define([_MOO_NET_VIABLE_ADD],
[[moo_net_cv_configs="${moo_net_cv_configs}]$2[,"
moo_net_uconfigs="$moo_net_uconfigs]
AS_HELP_STRING([[--]m4_ifval([$1],[[enable-net=]$1],[[disable-net]])dnl
m4_if(m4_index([$1],[tcp]),[-1],[],[[[,out]]])],
  $2[]m4_if(m4_index([$2],[TCP]),[-1],[],[[ [+ OUTBOUND_NETWORK]]]),[35])"])dnl

# ----------------------------------------------------------
#   viability test logic
#   notes:
#     autoconf caches all shell variables named with '_cv_'
#     MOO_NET_FIFO_WORKS sets moo_net_cv_fifo_*, and
#     AC_CHECK_HEADER sets ac_cv_header_*
#     AC_SEARCH_LIBS sets ac_cv_search_*
#
m4_define([_MOO_NET_VIABLE(NP_LOCAL/NS_BSD)],
[[test "$ac_cv_header_sys_socket_h" = yes &&
   test "$ac_cv_search_accept" != no]])dnl
dnl
m4_define([_MOO_NET_VIABLE(NP_TCP/NS_BSD)],
[[test "$ac_cv_header_sys_socket_h" = yes &&
   test "$ac_cv_search_accept" != no]])dnl
dnl
m4_define([_MOO_NET_VIABLE(NP_LOCAL/NS_SYSV)],
[[test "$moo_net_cv_fifo_select" = yes ||
   test "$moo_net_cv_fifo_fstat" = yes ||
   test "$moo_net_cv_fifo_poll" = yes]])dnl
dnl
m4_define([_MOO_NET_VIABLE(NP_TCP/NS_SYSV)],
[[test "$ac_cv_header_tiuser_h"  = yes &&
   test "$ac_cv_have_decl_t_open" = yes &&
   test "$ac_cv_have_decl_poll"   = yes &&
   test "$ac_cv_search_t_open" != no &&
   test -r /dev/tcp]])dnl


# ******************************************************************
#  MOO_NET_DO_REPORT
#    Tell user about the network configurations
#    and/or warn/error if non-viable
#    (this should be at/near the end of configure.ac).
#    depends on (:-vs-false) settings of
#      $moo_net_do_list    -> display viable configuration list
#      $moo_net_do_check   -> check chosen configuration
#      $moo_net_do_require -> error if dubious configuration
#
AC_DEFUN([MOO_NET_DO_REPORT],
  [AC_REQUIRE([MOO_NET_VIABLE])[
if $moo_net_do_list ; then
  cat <<_MOO_EOF
----------------------------------------------------------------------
| The following networking configurations will probably work on your
| system; other configurations will very likely fail:

configure argument:              corresponding options.h[.in] values:]dnl
[$moo_net_uconfigs
----------------------------------------------------------------------
_MOO_EOF
fi
if $moo_net_do_check ; then
  _moo_config=$enable_def_NETWORK_PROTOCOL
  test "$enable_def_NETWORK_STYLE" != no &&
    _moo_config=$_moo_config/$enable_def_NETWORK_STYLE
  _moo_fail=]
  AS_CASE([[$moo_net_cv_configs]],[[
    *,$_moo_config,*]],[
      AS_IF([[test $enable_def_OUTBOUND_NETWORK != no]],[
        AS_CASE([[$_moo_config]],[[
	  *TCP*]],[],
          [[_moo_fail="OUTBOUND_NETWORK not allowed for $_moo_config"]])
    ])],[[
      _moo_fail="$_moo_config is not viable"
    ]])
  AS_IF([[test "$_moo_fail"]],[
    AS_IF([$moo_net_do_require],
      [AC_MSG_ERROR([$_moo_fail])],
      [AC_MSG_WARN([$_moo_fail])
    ])
  ])[
fi
]])dnl
