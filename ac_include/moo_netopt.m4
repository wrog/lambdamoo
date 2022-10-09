# -*- autoconf -*-

# ******************************************************
#  MOO_NET_ENABLE_ARG([net])
#    creates the --enable-net=kwds... ./configure argument
#    sets shell vars for --enable-def-*
#      $moo_d_{NETWORK_PROTOCOL,NETWORK_STYLE,MPLEX_STYLE,
#              DEFAULT_{PORT,CONNECT_FILE},
#              OUTBOUND_NETWORK}
#    (yes this takes an argument
#    but it is probably a bad idea to change it.)
#
AC_DEFUN([MOO_NET_ARG_ENABLE],
[AC_ARG_ENABLE([$1],
[AS_HELP_STRING([--disable-$1],[NETWORK_PROTOCOL=NP_SINGLE])
AS_HELP_STRING([--enable-$1=KWD,KWD...],
                         [set network options. Allowed KWDs are])
[                  tcp, local:  NETWORK_PROTOCOL=NP_*]
[                   bsd, sysv:  NETWORK_STYLE=NS_*]
[          select, poll, fake:  MPLEX_STYLE=MP_*]
[  path/to/file, '"filename"':  DEFAULT_CONNECT_FILE=*]
[               [port number]:  DEFAULT_PORT=*]
[          noout, outoff, out:  OUTBOUND_NETWORK={undef,-O,+O}]],
[[ac_save_IFS=$IFS
IFS=,
for moo_kwd in x $enableval ; do
  IFS=$ac_save_IFS]
  AS_CASE([$moo_kwd],[
    x],     [[continue]],                               [
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
  AC_MSG_NOTICE([(--enable-$1:) $moo_d = $moo_v])
[done]])])
