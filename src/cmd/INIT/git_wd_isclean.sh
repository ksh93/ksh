########################################################################
#                                                                      #
#              This file is part of the ksh 93u+m package              #
#             Copyright (c) 2026 Contributors to ksh 93u+m             #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                                                                      #
########################################################################

# Consider the working directory clean if files that are relevant to ksh93
# have uncommitted changes. This determines whether to create a release or
# development build (libast/Mamfile) and whether to add /MOD to ksh's version
# string (ksh93/Mamfile).

git update-index --really-refresh >/dev/null  # may return status 1

exec git diff-index --quiet HEAD -- \
	':(top)src/lib/libast/*/*.[ch]' \
	':(top)src/lib/libast/*/*.tab' \
	':(top)sec/lib/libast/features' \
	':(top)src/lib/libdll/*.[ch]' \
	':(top)sec/lib/libdll/features' \
	':(top)src/lib/libcmd/*.[ch]' \
	':(top)sec/lib/libcmd/features' \
	':(top)src/cmd/ksh93/bltins' \
	':(top)src/cmd/ksh93/data' \
	':(top)src/cmd/ksh93/edit' \
	':(top)src/cmd/ksh93/features' \
	':(top)src/cmd/ksh93/include' \
	':(top)src/cmd/ksh93/sh'
