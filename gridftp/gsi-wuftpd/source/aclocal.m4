dnl aclocal.m4 generated automatically by aclocal 1.2

dnl GSSAPI_CONFIG
dnl
dnl Figure out our GSSAPI configuration
dnl

AC_DEFUN([GSSAPI_CONFIG],
[

AC_MSG_CHECKING(for type of gssapi support)

AC_ARG_WITH(gssapi,
[  --with-gssapi=<type>        Specify type of GSSAPI
                              Options are: krb5, globus, gsi],
[case $withval in
  no)	# No support
		AC_MSG_RESULT(none)
		;;
  krb5) # Kerberos 5 support
		AC_MSG_RESULT([Kerberos 5])
		;;
  gsi|globus) # Globus SSLeasy
		AC_MSG_RESULT([Globus/GSI SSLeay])
		# Make sure it is "globus" and not "gsi"
		with_gssapi="globus"
		;;
  *)	# Unrecognized type
		with_gssapi=no
		AC_MSG_ERROR(Unrecognized GSSAPI type)
		;;
esac],
[with_gssapi=no
AC_MSG_RESULT(none)
])

if test "$with_gssapi" != "no" ; then

	# Do stuff here for any GSSAPI library
	AC_DEFINE(GSSAPI)

	AC_ARG_WITH(gssapi-dir,
		[  --with-gssapi-dir=<DIR>  Specify install directory for GSSAPI package],
		gssapi_dir=$withval,
		gssapi_dir="none"
	)

fi

if test "$with_gssapi" = "globus" ; then

	# Globus GSSAPI configuration
	AC_DEFINE(GSSAPI_GLOBUS)

	# Find GLOBUS/GSI installation Directory
	AC_MSG_CHECKING(for Globus/GSI installation directory)

	globus_install_dir=$gssapi_dir

	if test "$globus_install_dir" = "none" ; then
		if test -n "$GLOBUS_INSTALL_PATH" ; then
			globus_install_dir=$GLOBUS_INSTALL_PATH
		elif test -d /usr/local/globus ; then
			globus_install_dir="/usr/local/globus"
		elif test -d /usr/local/gsi ; then
			globus_install_dir="/usr/local/gsi"
		else
			AC_MSG_ERROR(Cannot find Globus/GSI installation directory)
		fi	
	fi
	AC_MSG_RESULT($globus_install_dir)

	# Find GLOBUS/GSI development directory
	AC_MSG_CHECKING(for Globus/GSI development directory)

	if test -d ${globus_install_dir}/lib ; then
		# Looks like a flat directory structure from configure/make and not
		# globus-install or gsi-install
		globus_dev_dir=$globus_install_dir

	else
		# Assume a true globus installation with architecture
		# directories and run globus-development-path to find
		# the development directory

		# Make sure GLOBUS_INSTALL_PATH is set
		if test -z "$GLOBUS_INSTALL_PATH" ; then
			GLOBUS_INSTALL_PATH=$globus_install_dir
			export GLOBUS_INSTALL_PATH
		fi

		dev_path_program=${globus_install_dir}/bin/globus-development-path

		if test ! -x ${dev_path_program} ; then
			AC_MSG_ERROR(Cannot find Globus/GSI installation directory: program ${dev_path_program} does not exist or is not executable)
		fi

		globus_dev_dir=`${dev_path_program}`

		if test -z "$globus_dev_dir" -o "X$globus_dev_dir" = "X<not found>" ; then
				AC_MSG_ERROR(Cannot find Globus/GSI development directory)
		fi

		if test ! -d "$globus_dev_dir" ; then
			AC_MSG_ERROR(Cannot find Globus/GSI development directory: $globus_dev_dir does not exist)
		fi
	fi
	AC_MSG_RESULT($globus_dev_dir)

	GSSAPI_LIBS="-lglobus_gss_assist -lglobus_gss -lglobus_gaa"
	GSSAPI_LDFLAGS="-L${globus_dev_dir}/lib"
	GSSAPI_CFLAGS="-I${globus_dev_dir}/include"

	dnl Find SSLeay installation directory
	GSSAPI_LIBS="$GSSAPI_LIBS -lssl"

  AC_MSG_CHECKING(for ssleay directory)
  AC_ARG_WITH(ssl-dir,
     [  --with-ssl-dir=<DIR>  Root directory for ssleay stuff],
     ssleay_dir=$withval,
     ssleay_dir="/usr/local/ssl"
  )
  AC_MSG_RESULT($ssleay_dir)

  if test "$ssleay_dir" != "none" ; then
    GSSAPI_LDFLAGS="-L${ssleay_dir}/lib $GSSAPI_LDFLAGS"
    GSSAPI_CFLAGS="-I${ssleay_dir}/include $GSSAPI_CFLAGS"
		# Specify full path to ssleay's libcrypto so we don't conflict
		# with Keberos libcrypto.a
		GSSAPI_LIBS="$GSSAPI_LIBS ${ssleay_dir}/lib/libcrypto.a"
	else
		GSSAPI_LIBS="$GSSAPI_LIBS -lcrypto}"
  fi

  # End Globus/GSI section

elif test "$with_gssapi" = "krb5" ; then

	# Kerberos 5 GSSAPI configuration
	AC_DEFINE(GSSAPI_KRB5)

	# Find Kerberos 5 installation directory
	AC_MSG_CHECKING(for Krb5 installation directory)

	krb5_install_dir=$gssapi_dir

	if test "$krb5_install_dir" = "none" ; then
		if test -d /usr/local/krb5 ; then
			krb5_install_dir="/usr/local/krb5"
		elif test -d /krb5 ; then
			krb5_install_dir="/krb5"
		else
			AC_MSG_ERROR(Cannot find Kerberos 5 installation directory)
		fi	
	fi
	AC_MSG_RESULT($krb5_install_dir)

	GSSAPI_LIBS="-lgssapi_krb5 -lkrb5 -lcrypto -lcom_err"
  GSSAPI_LDFLAGS="-L${krb5_install_dir}/lib"
	# For <krb5.h>
	GSSAPI_CFLAGS="-I${krb5_install_dir}/include $GSSAPI_CFLAGS"
	# For <gssapi.h>
	GSSAPI_CFLAGS="-I${krb5_install_dir}/include/gssapi $GSSAPI_CFLAGS"

	# End Kerberos 5 Section
fi

AC_SUBST(GSSAPI_LIBS)
AC_SUBST(GSSAPI_LDFLAGS)
AC_SUBST(GSSAPI_CFLAGS)

if test "$with_gssapi" != "no" ; then
  dnl Check for the existance of specific GSSAPI routines.
  dnl Need to do this after GSSAPI_LIBS is completely filled out
  ORIG_LIBS="$LIBS"
  ORIG_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $GSSAPI_LDFLAGS"
  LIBS="$LIBS $GSSAPI_LIBS"

  AC_MSG_CHECKING(for gss_seal)
  AC_TRY_LINK([],[gss_seal();],
      [AC_MSG_RESULT(yes)
       AC_DEFINE(HAVE_GSS_SEAL)],
      AC_MSG_RESULT(no))

  AC_MSG_CHECKING(for gss_unseal)
  AC_TRY_LINK([],[gss_unseal();],
      [AC_MSG_RESULT(yes)
       AC_DEFINE(HAVE_GSS_UNSEAL)],
      AC_MSG_RESULT(no))

  AC_MSG_CHECKING(for gss_export_name)
  AC_TRY_LINK([],[gss_export_name();],
      [AC_MSG_RESULT(yes)
       AC_DEFINE(HAVE_GSS_EXPORT_NAME)],
      AC_MSG_RESULT(no))

  LIBS="$ORIG_LIBS"
  LDFLAGS="$ORIG_LDFLAGS"
fi

]) dnl AC_GSSAPI_CONFIG
