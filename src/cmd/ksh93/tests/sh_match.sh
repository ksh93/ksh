########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#                    Copyright (c) 2012 Roland Mainz                   #
#          Copyright (c) 2020-2023 Contributors to ksh 93u+m           #
#                      and is licensed under the                       #
#                 Eclipse Public License, Version 2.0                  #
#                                                                      #
#                A copy of the License is available at                 #
#      https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html      #
#         (with md5 checksum 84283fa8859daf213bdda5a9f8d1be1d)         #
#                                                                      #
#                    David Korn <dgkorn@gmail.com>                     #
#                Roland Mainz <roland.mainz@nrubsig.org>               #
#            Johnothan King <johnothanking@protonmail.com>             #
#                  Martijn Dekker <martijn@inlv.org>                   #
#                      Phi <phi.debian@gmail.com>                      #
#                                                                      #
########################################################################

#
# This test module tests the .sh.match pattern matching facility
#

. "${SHTESTS_COMMON:-${0%/*}/_common}"

# =====
# Start with basic character class matching tests backported from ksh2020. This
# is primarily to verify that the underlying AST regex code is working as
# expected before moving on to more complex tests.
[[ 1 =~ [[:digit:]] ]] || err_exit 'pattern [[:digit:]] broken'
[[ x =~ [[:digit:]] ]] && err_exit 'pattern [[:digit:]] broken'
[[ 5 =~ [[:alpha:]] ]] && err_exit 'pattern [[:alpha:]] broken'
[[ z =~ [[:alpha:]] ]] || err_exit 'pattern [[:alpha:]] broken'
[[ 3 =~ [[:alnum:]] ]] || err_exit 'pattern [[:alnum:]] broken'
[[ y =~ [[:alnum:]] ]] || err_exit 'pattern [[:alnum:]] broken'
[[ / =~ [[:alnum:]] ]] && err_exit 'pattern [[:alnum:]] broken'
[[ 3 =~ [[:lower:]] ]] && err_exit 'pattern [[:lower:]] broken'
[[ y =~ [[:lower:]] ]] || err_exit 'pattern [[:lower:]] broken'
[[ B =~ [[:lower:]] ]] && err_exit 'pattern [[:lower:]] broken'
[[ 3 =~ [[:upper:]] ]] && err_exit 'pattern [[:upper:]] broken'
[[ y =~ [[:upper:]] ]] && err_exit 'pattern [[:upper:]] broken'
[[ B =~ [[:upper:]] ]] || err_exit 'pattern [[:upper:]] broken'
[[ 7 =~ [[:word:]] ]] || err_exit 'pattern [[:word:]] broken'
[[ x =~ [[:word:]] ]] || err_exit 'pattern [[:word:]] broken'
[[ _ =~ [[:word:]] ]] || err_exit 'pattern [[:word:]] broken'
[[ + =~ [[:word:]] ]] && err_exit 'pattern [[:word:]] broken'
[[ . =~ [[:space:]] ]] && err_exit 'pattern [[:space:]] broken'
[[ X =~ [[:space:]] ]] && err_exit 'pattern [[:space:]] broken'
[[ ' ' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\t' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\v' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\f' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\n' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ . =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ X =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ ' ' =~ [[:blank:]] ]] || err_exit 'pattern [[:blank:]] broken'
[[ $'\t' =~ [[:blank:]] ]] || err_exit 'pattern [[:blank:]] broken'
[[ $'\v' =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ ' ' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\t' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\v' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\f' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ $'\n' =~ [[:space:]] ]] || err_exit 'pattern [[:space:]] broken'
[[ . =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ X =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ ' ' =~ [[:blank:]] ]] || err_exit 'pattern [[:blank:]] broken'
[[ $'\t' =~ [[:blank:]] ]] || err_exit 'pattern [[:blank:]] broken'
[[ $'\v' =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ $'\f' =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ $'\n' =~ [[:blank:]] ]] && err_exit 'pattern [[:blank:]] broken'
[[ Z =~ [[:print:]] ]] || err_exit 'pattern [[:print:]] broken'
[[ ' ' =~ [[:print:]] ]] || err_exit 'pattern [[:print:]] broken'
[[ $'\cg' =~ [[:print:]] ]] && err_exit 'pattern [[:print:]] broken'
[[ Z =~ [[:cntrl:]] ]] && err_exit 'pattern [[:cntrl:]] broken'
[[ ' ' =~ [[:cntrl:]] ]] && err_exit 'pattern [[:cntrl:]] broken'
[[ $'\cg' =~ [[:cntrl:]] ]] || err_exit 'pattern [[:cntrl:]] broken'
[[ \$ =~ [[:graph:]] ]] || err_exit 'pattern [[:graph:]] broken'
[[ ' ' =~ [[:graph:]] ]] && err_exit 'pattern [[:graph:]] broken'
[[ \$ =~ [[:punct:]] ]] || err_exit 'pattern [[:punct:]] broken'
[[ / =~ [[:punct:]] ]] || err_exit 'pattern [[:punct:]] broken'
[[ ' ' =~ [[:punct:]] ]] && err_exit 'pattern [[:punct:]] broken'
[[ x =~ [[:punct:]] ]] && err_exit 'pattern [[:punct:]] broken'
[[ ' ' =~ [[:xdigit:]] ]] && err_exit 'pattern [[:xdigit:]] broken'
[[ x =~ [[:xdigit:]] ]] && err_exit 'pattern [[:xdigit:]] broken'
[[ 0 =~ [[:xdigit:]] ]] || err_exit 'pattern [[:xdigit:]] broken'
[[ 9 =~ [[:xdigit:]] ]] || err_exit 'pattern [[:xdigit:]] broken'
[[ A =~ [[:xdigit:]] ]] || err_exit 'pattern [[:xdigit:]] broken'
[[ a =~ [[:xdigit:]] ]] || err_exit 'pattern [[:xdigit:]] broken'
[[ F =~ [[:xdigit:]] ]] || err_exit 'pattern [[:xdigit:]] broken'
[[ f =~ [[:xdigit:]] ]] || err_exit 'pattern [[:xdigit:]] broken'
[[ G =~ [[:xdigit:]] ]] && err_exit 'pattern [[:xdigit:]] broken'
[[ g =~ [[:xdigit:]] ]] && err_exit 'pattern [[:xdigit:]] broken'

[[ 3 =~ \w ]] || err_exit 'pattern \w broken'
[[ y =~ \w ]] || err_exit 'pattern \w broken'
[[ / =~ \w ]] && err_exit 'pattern \w broken'
[[ 3 =~ \W ]] && err_exit 'pattern \w broken'
[[ y =~ \W ]] && err_exit 'pattern \w broken'
[[ / =~ \W ]] || err_exit 'pattern \w broken'
[[ . =~ \s ]] && err_exit 'pattern \s broken'
[[ X =~ \s ]] && err_exit 'pattern \s broken'
[[ ' ' =~ \s ]] || err_exit 'pattern \s broken'
[[ $'\t' =~ \s ]] || err_exit 'pattern \s broken'
[[ $'\v' =~ \s ]] || err_exit 'pattern \s broken'
[[ $'\f' =~ \s ]] || err_exit 'pattern \s broken'
[[ $'\n' =~ \s ]] || err_exit 'pattern \s broken'
[[ x =~ \d ]] && err_exit 'pattern \d broken'
[[ 9 =~ \d ]] || err_exit 'pattern \d broken'
[[ x =~ \D ]] || err_exit 'pattern \D broken'
[[ 9 =~ \D ]] && err_exit 'pattern \D broken'
[[ 7 =~ \b ]] || err_exit 'pattern \b broken'
[[ x =~ \b ]] || err_exit 'pattern \b broken'
[[ _ =~ \b ]] || err_exit 'pattern \b broken'
[[ + =~ \b ]] || err_exit 'pattern \b broken'
[[ 'x y ' =~ .\b.\b ]] || err_exit 'pattern \b broken'
[[ ' xy ' =~ .\b.\b ]] && err_exit 'pattern \b broken'
[[ 7 =~ \B ]] && err_exit 'pattern \B broken'
[[ x =~ \B ]] && err_exit 'pattern \B broken'
[[ _ =~ \B ]] && err_exit 'pattern \B broken'
[[ + =~ \B ]] || err_exit 'pattern \B broken'

# ======
# Tests backported from ksh93v-
function test_xmlfragment1
{
	typeset -r testscript='test1_script.sh'
cat >"${testscript}" <<-TEST1SCRIPT
	# input text
	xmltext="\$( < "\$1" )"

	print -f "%d characters to process...\\n" "\${#xmltext}"

	#
	# parse the XML data
	#
	typeset dummy
	function parse_xmltext
	{
		typeset xmltext="\$2"
		nameref ar="\$1"

		# fixme:
		# - We want to enforce standard conformance - does ~(Exp) or ~(Ex-p) do that ?
		dummy="\${xmltext//~(Ex-p)(?:
			(<!--.*-->)+?|			# xml comments
			(<[:_[:alnum:]-]+
				(?: # attributes
					[[:space:]]+
					(?: # four different types of name=value syntax
						(?:[:_[:alnum:]-]+=[^\\"\\'[:space:]]+?)|	#x='foo=bar huz=123'
						(?:[:_[:alnum:]-]+=\\"[^\\"]*?\\")|		#x='foo="ba=r o" huz=123'
						(?:[:_[:alnum:]-]+=\\'[^\\']*?\\')|		#x="foox huz=123"
						(?:[:_[:alnum:]-]+)				#x="foox huz=123"
					)
				)*
				[[:space:]]*
				\\/?	# start tags which are end tags, too (like <foo\\/>)
			>)+?|				# xml start tags
			(<\\/[:_[:alnum:]-]+>)+?|	# xml end tags
			([^<]+)				# xml text
			)/D}"

		# copy ".sh.match" to array "ar"
		integer i j
		for i in "\${!.sh.match[@]}" ; do
			for j in "\${!.sh.match[i][@]}" ; do
				[[ -v .sh.match[i][j] ]] && ar[i][j]="\${.sh.match[i][j]}"
			done
		done

		return 0
	}

	function rebuild_xml_and_verify
	{
		nameref ar="\$1"
		typeset xtext="\$2" # xml text

		#
		# rebuild the original text from "ar" (copy of ".sh.match")
		# and compare it to the content of "xtext"
		#
		tmpfile=rebuild_xml_and_verify.\$\$

		{
			# rebuild the original text, based on our matches
			nameref nodes_all=ar[0]		# contains all matches
			nameref nodes_comments=ar[1]	# contains only XML comment matches
			nameref nodes_start_tags=ar[2]	# contains only XML start tag matches
			nameref nodes_end_tags=ar[3]	# contains only XML end tag matches
			nameref nodes_text=ar[4]	# contains only XML text matches
			integer i
			for (( i = 0 ; i < \${#nodes_all[@]} ; i++ )) ; do
				[[ -v nodes_comments[i]		]] && printf '%s' "\${nodes_comments[i]}"
				[[ -v nodes_start_tags[i]	]] && printf '%s' "\${nodes_start_tags[i]}"
				[[ -v nodes_end_tags[i]		]] && printf '%s' "\${nodes_end_tags[i]}"
				[[ -v nodes_text[i]		]] && printf '%s' "\${nodes_text[i]}"
			done
			printf '\\n'
		} >"\${tmpfile}"

		diff -u <( printf '%s\\n' "\${xtext}") "\${tmpfile}" | sed '/No differences encountered/d'
		if cmp <( printf '%s\\n' "\${xtext}") "\${tmpfile}" ; then
			printf "#input and output OK (%d characters).\\n" "\$(wc -m <"\${tmpfile}")"
		else
			printf "#difference between input and output found.\\n"
		fi

		rm -f "\${tmpfile}"
		return 0
	}

	# main
	set -o nounset

	typeset -a xar
	parse_xmltext xar "\$xmltext"
	rebuild_xml_and_verify xar "\$xmltext"
TEST1SCRIPT

cat >'testfile1.xml' <<-EOF
	<refentry>
		<refentryinfo>
			<title>&dhtitle;</title>
			<productname>&dhpackage;</productname>
			<releaseinfo role="version">&dhrelease;</releaseinfo>
			<date>&dhdate;</date>
			<authorgroup>
				<author>
					<firstname>XXXX</firstname>
					<surname>YYYYYYYYYYYY</surname>
					<contrib>Wrote this example manpage for the &quot;SunOS Man Page Howto&quot;, available at <ulink url="http://www.YYYYYYYYYYYY.xxx/foo_batt_12345.abcd"/> or <ulink url="http://www.1234.xxx/info/SunOS-mini/123-4567.hhhh"/>.</contrib>
					<address>
						<email>mailmail@YYYYYYYYYYYY.xxx</email>
					</address>
				</author>
				<author>
					<firstname>&dhfirstname;</firstname>
					<surname>&dhsurname;</surname>
					<contrib>Rewrote and extended the example manpage in DocBook XML for the Zebras distribution.</contrib>
					<address>
						<email>&dhemail;</email>
					</address>
				</author>
			</authorgroup>
			<copyright>
				<year>1995</year>
				<year>1996</year>
				<year>1997</year>
				<year>1998</year>
				<year>1999</year>
				<year>2000</year>
				<year>2001</year>
				<year>2002</year>
				<year>2003</year>
				<holder>XXXX YYYYYYYYYYYY</holder>
			</copyright>
			<copyright>
				<year>2006</year>
				<holder>&dhusername;</holder>
			</copyright>
			<legalnotice>
				<para>The Howto containing this example, was offered under the following conditions:</para>
				<para>Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:</para>
				<orderedlist>
					<listitem>
						<para>Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.</para>
					</listitem>
					<listitem>
						<para>Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.</para>
					</listitem>
				</orderedlist>
				<para>THIS SOFTWARE IS PROVIDED BY THE AUTHOR &quot;AS IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</para>
			</legalnotice>
		</refentryinfo>
		<refmeta>
			<refentrytitle>&dhucpackage;</refentrytitle>
			<manvolnum>&dhsection;</manvolnum>
		</refmeta>
		<refnamediv>
			<refname>&dhpackage;</refname>
			<refpurpose>frobnicate the bar library</refpurpose>
		</refnamediv>
		<refsynopsisdiv>
			<cmdsynopsis>
				<command>&dhpackage;</command>
				<arg choice="opt"><option>-bar</option></arg>
				<group choice="opt">
					<arg choice="plain"><option>-b</option></arg>
					<arg choice="plain"><option>--busy</option></arg>
				</group>
				<group choice="opt">
					<arg choice="plain"><option>-c <replaceable>config-file</replaceable></option></arg>
					<arg choice="plain"><option>--config=<replaceable>config-file</replaceable></option></arg>
				</group>
				<arg choice="opt">
					<group choice="req">
						<arg choice="plain"><option>-e</option></arg>
						<arg choice="plain"><option>--example</option></arg>
					</group>
					<replaceable class="option">this</replaceable>
				</arg>
				<arg choice="opt">
					<group choice="req">
						<arg choice="plain"><option>-e</option></arg>
						<arg choice="plain"><option>--example</option></arg>
					</group>
					<group choice="req">
						<arg choice="plain"><replaceable>this</replaceable></arg>
						<arg choice="plain"><replaceable>that</replaceable></arg>
					</group>
				</arg>
				<arg choice="plain" rep="repeat"><replaceable>file(s)</replaceable></arg>
			</cmdsynopsis>
			<cmdsynopsis>
				<command>&dhpackage;</command>
	      <!-- Normally the help and version options make the programs stop
				     right after outputting the requested information. -->
				<group choice="opt">
					<arg choice="plain">
						<group choice="req">
							<arg choice="plain"><option>-h</option></arg>
							<arg choice="plain"><option>--help</option></arg>
						</group>
					</arg>
					<arg choice="plain">
						<group choice="req">
							<arg choice="plain"><option>-v</option></arg>
							<arg choice="plain"><option>--version</option></arg>
						</group>
					</arg>
				</group>
			</cmdsynopsis>
		</refsynopsisdiv>
		<refsect1 id="description">
			<title>DESCRIPTION</title>
			<para><command>&dhpackage;</command> frobnicates the <application>bar</application> library by tweaking internal symbol tables. By default it parses all baz segments and rearranges them in reverse order by time for the <citerefentry><refentrytitle>xyzzy</refentrytitle><manvolnum>1</manvolnum></citerefentry> linker to find them. The symdef entry is then compressed using the <abbrev>WBG</abbrev> (Whiz-Bang-Gizmo) algorithm. All files are processed in the order specified.</para>
		</refsect1>
		<refsect1 id="options">
			<title>OPTIONS</title>
			<variablelist>
				<!-- Use the variablelist.term.separator and the
				     variablelist.term.break.after parameters to
				     control the term elements. -->
				<varlistentry>
					<term><option>-b</option></term>
					<term><option>--busy</option></term>
					<listitem>
						<para>Do not write <quote>busy</quote> to <filename class="devicefile">stdout</filename> while processing.</para>
					</listitem>
				</varlistentry>
				<varlistentry>
					<term><option>-c <replaceable class="parameter">config-file</replaceable></option></term>
					<term><option>--config=<replaceable class="parameter">config-file</replaceable></option></term>
					<listitem>
						<para>Use the alternate system wide <replaceable>config-file</replaceable> instead of the <filename>/etc/foo.conf</filename>. This overrides any <envar>FOOCONF</envar> environment variable.</para>
					</listitem>
				</varlistentry>
				<varlistentry>
					<term><option>-a</option></term>
					<listitem>
						<para>In addition to the baz segments, also parse the <citerefentry><refentrytitle>blurfl</refentrytitle><manvolnum>3</manvolnum></citerefentry> headers.</para>
					</listitem>
				</varlistentry>
				<varlistentry>
					<term><option>-r</option></term>
					<listitem>
						<para>Recursive mode. Operates as fast as lightning at the expense of a megabyte of virtual memory.</para>
					</listitem>
				</varlistentry>
			</variablelist>
		</refsect1>
		<refsect1 id="files">
			<title>FILES</title>
			<variablelist>
				<varlistentry>
					<term><filename>/etc/foo.conf</filename></term>
					<listitem>
						<para>The system-wide configuration file. See <citerefentry><refentrytitle>foo.conf</refentrytitle><manvolnum>5</manvolnum></citerefentry> for further details.</para>
					</listitem>
				</varlistentry>
				<varlistentry>
					<term><filename>\${HOME}/.foo.conf</filename></term>
					<listitem>
						<para>The per-user configuration file. See <citerefentry><refentrytitle>foo.conf</refentrytitle><manvolnum>5</manvolnum></citerefentry> for further details.</para>
					</listitem>
				</varlistentry>
			</variablelist>
		</refsect1>
		<refsect1 id="environment">
			<title>ENVIRONMENT</title>
			<variablelist>
				<varlistentry>
				<term><envar>FOOCONF</envar></term>
					<listitem>
						<para>The full pathname for an alternate system wide configuration file <citerefentry><refentrytitle>foo.conf</refentrytitle><manvolnum>5</manvolnum></citerefentry> (see also <xref linkend="files"/>). Overridden by the <option>-c</option> option.</para>
					</listitem>
				</varlistentry>
			</variablelist>
		</refsect1>
		<refsect1 id="diagnostics">
			<title>DIAGNOSTICS</title>
			<para>The following diagnostics may be issued on <filename class="devicefile">stderr</filename>:</para>
			<variablelist>
				<varlistentry>
					<term><quote><errortext>Bad magic number.</errortext></quote></term>
					<listitem>
						<para>The input file does not look like an archive file.</para>
					</listitem>
				</varlistentry>
				<varlistentry>
					<term><quote><errortext>Old style baz segments.</errortext></quote></term>
					<listitem>
						<para><command>&dhpackage;</command> can only handle new style baz segments. <acronym>COBOL</acronym> object libraries are not supported in this version.</para>
					</listitem>
				</varlistentry>
			</variablelist>
			<para>The following return codes can be used in scripts:</para>
			<segmentedlist>
				<segtitle>Errorcode</segtitle>
				<segtitle>Errortext</segtitle>
				<segtitle>Diagnostic</segtitle>
				<seglistitem>
					<seg><errorcode>0</errorcode></seg>
					<seg><errortext>Program exited normally.</errortext></seg>
					<seg>No error. Program ran successfully.</seg>
				</seglistitem>
				<seglistitem>
					<seg><errorcode>1</errorcode></seg>
					<seg><errortext>Bad magic number.</errortext></seg>
					<seg>The input file does not look like an archive file.</seg>
				</seglistitem>
				<seglistitem>
					<seg><errorcode>2</errorcode></seg>
					<seg><errortext>Old style baz segments.</errortext></seg>
					<seg><command>&dhpackage;</command> can only handle new style baz segments. <acronym>COBOL</acronym> object libraries are not supported in this version.</seg>
				</seglistitem>
			</segmentedlist>
		</refsect1>
		<refsect1 id="bugs">
			<!-- Or use this section to tell about upstream BTS. -->
			<title>BUGS</title>
			<para>The command name should have been chosen more carefully to reflect its purpose.</para>
			<para>The upstreams <acronym>BTS</acronym> can be found at <ulink url="http://bugzilla.foo.tld"/>.</para>
		</refsect1>
		<refsect1 id="see_also">
			<title>SEE ALSO</title>
			<!-- In alphabetical order. -->
			<para><citerefentry>
					<refentrytitle>bar</refentrytitle>
					<manvolnum>1</manvolnum>
				</citerefentry>, <citerefentry>
					<refentrytitle>foo</refentrytitle>
					<manvolnum>1</manvolnum>
				</citerefentry>, <citerefentry>
					<refentrytitle>foo.conf</refentrytitle>
					<manvolnum>5</manvolnum>
				</citerefentry>, <citerefentry>
					<refentrytitle>xyzzy</refentrytitle>
					<manvolnum>1</manvolnum>
				</citerefentry></para>
			<para>The programs are documented fully by <citetitle>The Rise and Fall of a Fooish Bar</citetitle> available via the <application>Info</application> system.</para>
		</refsect1>
	</refentry>
EOF

# Note: Standalone '>' is valid XML text
printf "%s" $'<h1 style=\'nice\' h="bar">> <oook:banana color="<yellow />"><oook:apple-mash color="<green />"><div style="some green"><illegal tag /><br /> a text </div>More [TEXT].<!-- a comment (<disabled>) --></h1>' >'testfile2.xml'

	compound -r -a tests=(
		(
			file='testfile1.xml'
			expected_output=$'9764 characters to process...\n#input and output OK (9765 characters).'
		)
		(
			file='testfile2.xml'
			expected_output=$'201 characters to process...\n#input and output OK (202 characters).'
		)
	)
	compound out=( typeset stdout stderr ; integer res )
	integer i
	typeset expected_output
	typeset testname

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		nameref tst=tests[i]
		testname="${0}/${i}/${tst.file}"
		expected_output="${tst.expected_output}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset "${testscript}" "${tst.file}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${expected_output}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${expected_output}" ;}, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
	done

	rm "${testscript}"
	rm 'testfile1.xml'
	rm 'testfile2.xml'

	return 0
}

# test whether the [[ -v .sh.match[x][y] ]] operator works, try1
function test_testop_v1
{
	compound out=( typeset stdout stderr ; integer res )
	integer i
	typeset testname
	typeset expected_output

	compound -r -a tests=(
		(
			cmd='s="aaa bbb 333 ccc 555" ; s="${s//~(E)([[:alpha:]]+)|([[:digit:]]+)/NOP}" ;                   [[ -v .sh.match[2][3]   ]] || print "OK"'
			expected_output='OK'
		)
		(
			cmd='s="aaa bbb 333 ccc 555" ; s="${s//~(E)([[:alpha:]]+)|([[:digit:]]+)/NOP}" ; integer i=2 j=3 ; [[ -v .sh.match[$i][$j] ]] || print "OK"'
			expected_output='OK'
		)
		(
			cmd='s="aaa bbb 333 ccc 555" ; s="${s//~(E)([[:alpha:]]+)|([[:digit:]]+)/NOP}" ; integer i=2 j=3 ; [[ -v .sh.match[i][j]   ]] || print "OK"'
			expected_output='OK'
		)
	)

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		nameref tst=tests[i]
		testname="${0}/${i}/${tst.cmd}"
		expected_output="${tst.expected_output}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -c "${tst.cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${expected_output}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${expected_output}" ;}, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
	done

	return 0
}

# test whether the [[ -v .sh.match[x][y] ]] operator works, try2
function test_testop_v2
{
	compound out=( typeset stdout stderr ; integer res )
	integer i
	integer j
	integer j
	typeset testname
	typeset cmd

	compound -r -a tests=(
		(
			cmd='s="aaa bbb 333 ccc 555" ; s="${s//~(E)([[:alpha:]]+)|([[:digit:]]+)/NOP}"'
			integer y=6
			expected_output_1d=$'[0]\n[1]\n[2]'
			expected_output_2d=$'[0][0]\n[0][1]\n[0][2]\n[0][3]\n[0][4]\n[1][0]\n[1][1]\n[1][3]\n[2][2]\n[2][4]'
		)
		# FIXME: Add more hideous horror tests here
	)

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		nameref tst=tests[i]

		#
		# test first dimension, by plain number
		#
		cmd="${tst.cmd}"
		for (( j=0 ; j < tst.y ; j++ )) ; do
			cmd+="; $( printf "[[ -v .sh.match[%d] ]] && print '[%d]'\n" j j )"
		done
		cmd+='; true'

		testname="${0}/${i}/plain_number_index_1d/${cmd}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -c "${cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${tst.expected_output_1d}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${tst.expected_output_1d}" ;}, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"


		#
		# test second dimension, by plain number
		#
		cmd="${tst.cmd}"
		for (( j=0 ; j < tst.y ; j++ )) ; do
			for (( k=0 ; k < tst.y ; k++ )) ; do
				cmd+="; $( printf "[[ -v .sh.match[%d][%d] ]] && print '[%d][%d]'\n" j k j k )"
			done
		done
		cmd+='; true'

		testname="${0}/${i}/plain_number_index_2d/${cmd}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -c "${cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${tst.expected_output_2d}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${tst.expected_output_2d}" ;}, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"

		#
		# test first dimension, by variable index
		#
		cmd="${tst.cmd} ; integer i"
		for (( j=0 ; j < tst.y ; j++ )) ; do
			cmd+="; $( printf "(( i=%d )) ; [[ -v .sh.match[i] ]] && print '[%d]'\n" j j )"
		done
		cmd+='; true'

		testname="${0}/${i}/variable_index_1d/${cmd}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -c "${cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${tst.expected_output_1d}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${tst.expected_output_1d}" ;}, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"


		#
		# test second dimension, by variable index
		#
		cmd="${tst.cmd} ; integer i j"
		for (( j=0 ; j < tst.y ; j++ )) ; do
			for (( k=0 ; k < tst.y ; k++ )) ; do
				cmd+="; $( printf "(( i=%d , j=%d )) ; [[ -v .sh.match[i][j] ]] && print '[%d][%d]'\n" j k j k )"
			done
		done
		cmd+='; true'

		testname="${0}/${i}/variable_index_2d/${cmd}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -c "${cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${tst.expected_output_2d}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${tst.expected_output_2d}" ;}, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"

	done

	return 0
}

# test whether ${#.sh.match[0][@]} returns the right number of elements
function test_num_elements1
{
	compound out=( typeset stdout stderr ; integer res )
	integer i
	typeset testname
	typeset expected_output

	compound -r -a tests=(
		(
			cmd='s="a1a2a3" ; d="${s//~(E)([[:alpha:]])|([[:digit:]])/dummy}" ; printf "num=%d\n" "${#.sh.match[0][@]}"'
			expected_output='num=6'
		)
		(
			cmd='s="ababab" ; d="${s//~(E)([[:alpha:]])|([[:digit:]])/dummy}" ; printf "num=%d\n" "${#.sh.match[0][@]}"'
			expected_output='num=6'
		)
		(
			cmd='s="123456" ; d="${s//~(E)([[:alpha:]])|([[:digit:]])/dummy}" ; printf "num=%d\n" "${#.sh.match[0][@]}"'
			expected_output='num=6'
		)
	)

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		nameref tst=tests[i]
		testname="${0}/${i}/${tst.cmd}"
		expected_output="${tst.expected_output}"

		out.stderr="${ { out.stdout="${ ${SHELL} -o nounset -c "${tst.cmd}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

		[[ "${out.stdout}" == "${expected_output}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${expected_output}" ; }, got ${ printf '%q\n' "${out.stdout}" ; }"
		[[ "${out.stderr}" == ''		   ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
		(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
	done

	return 0
}

# dgk's test which checks whether typeset -m (rename variable) works for .sh.match
function test_shmatch_varmove_dgk1
{
	typeset out
	# we use an array of $'...\n' here to get correct line numbers
	typeset -r -a script=(
		$'set -o nounset\n'
		$'x=1234\n'
		$'compound co\n'
		$': "${x//~(X)([012])|([345])/ }"\n'
		$'x="$(print -v .sh.match)"\n'
		$'typeset -m co.array=.sh.match\n'
		$'y="$(print -v co.array)"\n'
		$'[[ "$y" == "$x" ]] && print "MATCH"\n'

# fixme: this currently outputs as ${co.array[2][(null)]}, which isn't correct
#		# added later by gisburn
#		$'printf "%s" "${co.array[2][1]}"'
	)

	out="$(${SHELL} -c "${script[*]}" 2>&1 ; print -- "$?")"

	[[ "${out}" == $'MATCH\n0' ]] || err_exit "${0}: typeset -m of .sh.match to variable not working, expected 'MATCH', got ${ printf '%q\n' "${out}" ; }"

	return 0
}

function test_nomatch_dgk1
{
cat >'testscript1.sh' <<'EOF'
	integer j k
	compound c
	compound -a c.attrs

	attrdata=$' x=\'1\' y=\'2\' z="3" end="world"'
	dummy="${attrdata//~(Ex-p)(?:
		[[:space:]]+
		( # four different types of name=value syntax
			(?:([:_[:alnum:]-]+)=([^\"\'[:space:]]+?))|	#x='foo=bar huz=123'
			(?:([:_[:alnum:]-]+)=\"([^\"]*?)\")|		#x='foo="ba=r o" huz=123'
			(?:([:_[:alnum:]-]+)=\'([^\']*?)\')|		#x="foox huz=123"
			(?:([:_[:alnum:]-]+))				#x="foox huz=123"
		)
		)/D}"
	for (( j=0 ; j < ${#.sh.match[0][@]} ; j++ ))
	do
		if [[ -v .sh.match[2][j] && -v .sh.match[3][j] ]]
		then	c.attrs+=( name="${.sh.match[2][j]}" value="${.sh.match[3][j]}" )
		fi
		if [[ -v .sh.match[4][j] && -v .sh.match[5][j] ]]
		then	c.attrs+=( name="${.sh.match[4][j]}" value="${.sh.match[5][j]}" )
		fi
		if [[ -v .sh.match[6][j] && -v .sh.match[7][j] ]] ; then
			c.attrs+=( name="${.sh.match[6][j]}" value="${.sh.match[7][j]}" )
		fi
	done
	print -v c
EOF
	expect='(
	typeset -a attrs=(
		[0]=(
			name=x
			value=1
		)
		[1]=(
			name=y
			value=2
		)
		[2]=(
			name=z
			value=3
		)
		[3]=(
			name=end
			value=world
		)
	)
)'
	compound out=( typeset stdout stderr ; integer res )
	typeset testname

	# plain
	testname="${0}/plain"
	out.stderr="${ { out.stdout="${ ${SHELL} -o nounset 'testscript1.sh' ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

	[[ "${out.stdout}" == "${expect}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${expect}" ; }, got ${ printf '%q\n' "${out.stdout}" ; }"
	[[ "${out.stderr}" == ''	  ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
	(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"

	# compiled
	testname="${0}/compiled"
	out.stderr="${ { out.stdout="${ ${SHCOMP} -n 'testscript1.sh' 'testscript1.shbin' ; ${SHELL} -o nounset 'testscript1.shbin' ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

	[[ "${out.stdout}" == "${expect}" ]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${expect}" ; }, got ${ printf '%q\n' "${out.stdout}" ; }"
	[[ "${out.stderr}" == ''	  ]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
	(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"

	rm 'testscript1.sh' 'testscript1.shbin'

	return 0
}

function test_sh_match_varmove2
{
cat >'testscript1.sh' <<EOF
function parse_attr
{
	typeset move_mode=\$1
	typeset attrdata=" \$2" # leading space is intentional to get eregex to work below
	integer i

	typeset dummy="\${attrdata//~(Ex-p)(?:
	[[:space:]]+
	( # four different types of name=value syntax
		(?:([:_[:alnum:]-]+)=([^\"\'[:space:]]+?))|
		(?:([:_[:alnum:]-]+)=\"([^\"]*?)\")|
		(?:([:_[:alnum:]-]+)=\'([^\']*?)\')|
		(?:([:_[:alnum:]-]+))
	)
	)/D}"

	case \${move_mode} in
		'plain')
			nameref m=.sh.match
			;;
		'move')
			typeset -m m=.sh.match
			;;
		'move_to_compound')
			compound mc
			typeset -m mc.m=.sh.match
			nameref m=mc.m
			;;
		'move_to_nameref_compound')
			compound mc
			nameref mcn=mc
			typeset -m mcn.m=.sh.match
			nameref m=mcn.m
			;;
		*)
			print -u2 -f '# wrong move_mode=%q\n' "\${move_mode}"
			return 1
			;;
	esac

	for (( i=0 ; i < \${#m[0][@]} ; i++ )) ; do
		[[ -v m[2][i] && -v m[3][i] ]] && printf '%q=%q\n' "\${m[2][i]}" "\${m[3][i]}"
		[[ -v m[4][i] && -v m[5][i] ]] && printf '%q=%q\n' "\${m[4][i]}" "\${m[5][i]}"
		[[ -v m[6][i] && -v m[7][i] ]] && printf '%q=%q\n' "\${m[6][i]}" "\${m[7][i]}"
	done
	print "Nummatches=\${#m[0][@]}"

	return 0
}

set -o nounset

parse_attr "\$1" "\$2"

exit \$?
EOF
	compound -r -a tests=(
		( attrstr=$'aname="avalue" x="y"'	  output=$'aname=avalue\nx=y\nNummatches=2' )
		( attrstr=$'aname=\'avalue\' x=\'y\''	  output=$'aname=avalue\nx=y\nNummatches=2' )
		( attrstr=$'aname="avalue" x=\'y\''	  output=$'aname=avalue\nx=y\nNummatches=2' )
		( attrstr=$'aname=\'avalue\' x="y"'	  output=$'aname=avalue\nx=y\nNummatches=2' )
		( attrstr=$'aname="avalue"'		  output=$'aname=avalue\nNummatches=1' )
	)
	compound out=( typeset stdout stderr ; integer res )
	typeset testname
	typeset mode
	integer numtests=0

	${SHCOMP} -n 'testscript1.sh' 'testscript1.shbin' || err_exit "${0}: shcomp failed with exit code $?."

	for (( i=0 ; i < ${#tests[@]} ; i++ )) ; do
		nameref tst=tests[$i] # fixme: this should be tst=tests[i]

		for mode in 'plain' 'move' 'move_to_compound' 'move_to_nameref_compound' ; do
			# plain
			testname="${0}/${i}/${mode}/plain"
			out.stderr="${ { out.stdout="${ ${SHELL} 'testscript1.sh' ${mode} "${tst.attrstr}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

			[[ "${out.stdout}" == "${tst.output}"	]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${tst.output}" ; }, got ${ printf '%q\n' "${out.stdout}" ; }"
			[[ "${out.stderr}" == ''		]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
			(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
			(( numtests++ ))

			# compiled
			testname="${0}/${i}/${mode}/compiled"
			out.stderr="${ { out.stdout="${ ${SHELL} 'testscript1.shbin' ${mode} "${tst.attrstr}" ; (( out.res=$? )) ; }" ; } 2>&1 ; }"

			[[ "${out.stdout}" == "${tst.output}"	]] || err_exit "${testname}: Expected stdout==${ printf '%q\n' "${tst.output}" ; }, got ${ printf '%q\n' "${out.stdout}" ; }"
			[[ "${out.stderr}" == ''		]] || err_exit "${testname}: Expected empty stderr, got ${ printf '%q\n' "${out.stderr}" ; }"
			(( out.res == 0 )) || err_exit "${testname}: Unexpected exit code ${out.res}"
			(( numtests++ ))
		done
	done

	rm 'testscript1.sh' 'testscript1.shbin'

	# safeguard against malfunctions in the test chain
	(( numtests == 40 )) || err_exit "${0}: Internal test script error, expected numtests == 40, got ${numtests}"

	return 0
}

# run tests
test_xmlfragment1
test_testop_v1
test_testop_v2
test_num_elements1
test_shmatch_varmove_dgk1
test_sh_match_varmove2
test_nomatch_dgk1

# ======
set +u
x=1234
compound co
: "${x//~(X)([012])|([345])/ }"
x=$(print -v .sh.match)
typeset -m co.array=.sh.match
y=$(print -v co.array)
[[ $y == "$x" ]] || 'typeset -m of .sh.match to variable not working'

# ======
# https://github.com/ksh93/ksh/issues/308
exp='typeset -a .sh.match=((1 2 3 4) (1 2) ([2]=3 [3]=4) )
typeset -a .sh.match[1]=(1 2)
typeset -a .sh.match[2]=([2]=3 [3]=4)
3 2 2
2 3'
got=$("$SHELL" -c '
	x=1234
	true ${x//~(X)([012])|([345])/ }
	typeset -p .sh.match .sh.match[1] .sh.match[2]
	echo ${#.sh.match[@]} ${#.sh.match[1][@]} ${#.sh.match[2][@]}
	echo ${!.sh.match[2][@]};
')
[[ $exp == "$got" ]] || err_exit "listing .sh.match indexed array results doesn't work correctly" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# https://marc.info/?l=ast-developers&m=134604855504311&w=2
nummatches=$tmp/nummatches.sh
cat > "$nummatches" << 'EOF'
attrdata=$' aname=avalue '

dummy="${attrdata//~(Ex-p)(?:
[[:space:]]+
( # four different types of name=value syntax
	(?:([:_[:alnum:]-]+)=([^\"\'[:space:]]+?))|
	(?:([:_[:alnum:]-]+)=\"([^\"]*?)\")|
	(?:([:_[:alnum:]-]+)=\'([^\']*?)\')|
	(?:([:_[:alnum:]-]+))
)
)/D}"

print -v .sh.match
print "Nummatches=${#.sh.match[0][@]}"
EOF
exp=$'(
	(
		\' aname=a\'
	)
	(
		aname\\=a
	)
	(
		aname
	)
	(
		a
	)
)
Nummatches=1'
got=$("$SHELL" "$nummatches")
[[ $exp == "$got" ]] || err_exit "Nummatches should be one" \
	"(expected $(printf %q "$exp"), got $(printf %q "$got"))"

# https://marc.info/?l=ast-developers&m=134490505607093
if ((SHOPT_NAMESPACE)); then
	type_nameref=$tmp/ksh93v_typeset_T_nameref_fails001.sh
	cat > "$type_nameref" << 'EOF'
	namespace xmlfragmentparser
	{
	typeset -T parser_t=(
		typeset -a data		# "raw" data from .sh.match
		compound -a context	# parsed tag data

		function build_context
		{
			typeset dummy
			typeset attrdata # data after "<tag" ...

			integer i
			for (( i=0 ; i < ${#_.data[@]} ; i++ )) ; do
				nameref currc=_.context[i] # current context

				dummy="${_.data[i]/~(El)<([:_[:alnum:]-]+)(.*)>/X}"
				currc.tagname="${.sh.match[1]}"
				attrdata="${.sh.match[2]}"

				if [[ "${attrdata}" != ~(Elr)[[:space:]]* ]] ; then
					dummy="${attrdata//~(Ex-p)(?:
						[[:space:]]+
						( # four different types of name=value syntax
							(?:([:_[:alnum:]-]+)=([^\"\'[:space:]]+?))|	#x='foo=bar huz=123'
							(?:([:_[:alnum:]-]+)=\"([^\"]*?)\")|		#x='foo="ba=r o" huz=123'
							(?:([:_[:alnum:]-]+)=\'([^\']*?)\')|		#x="foox huz=123"
							(?:([:_[:alnum:]-]+))				#x="foox huz=123"
						)
					)/D}"

					integer j k
					compound -a currc.attrs
					for (( j=0 ; j < ${#.sh.match[0][@]} ; j++ )) ; do
						if [[ -v .sh.match[2][j] && -v .sh.match[3][j] ]] ; then
							currc.attrs+=( name="${.sh.match[2][j]}" value="${.sh.match[3][j]}" )
						fi ; if [[ -v .sh.match[4][j] && -v .sh.match[5][j] ]] ; then
							currc.attrs+=( name="${.sh.match[4][j]}" value="${.sh.match[5][j]}" )
						fi ; if [[ -v .sh.match[6][j] && -v .sh.match[7][j] ]] ; then
							currc.attrs+=( name="${.sh.match[6][j]}" value="${.sh.match[7][j]}" )
						fi
					done
				fi
			done
			return 0
		}
	)
	}

	function main
	{
		.xmlfragmentparser.parser_t xd # xml document
		xd.data=( "<foo x='1' y='2' />" "<bar a='1' b='2' />" )
		xd.build_context
		print "$xd"
		return 0
	}

	# main
	set -o nounset
	main
EOF
	exp="(
	typeset -a data=(
		$'<foo x=\\'1\\' y=\\'2\\' />'
		$'<bar a=\\'1\\' b=\\'2\\' />'
	)
	typeset -C -a context=(
		[0]=(
			typeset -a attrs=(
				[0]=(
					name=x
					value=1
				)
				[1]=(
					name=y
					value=2
				)
			)
			tagname=foo
		)
		[1]=(
			typeset -a attrs=(
				[0]=(
					name=a
					value=1
				)
				[1]=(
					name=b
					value=2
				)
			)
			tagname=bar
		)
	)
)"
	got=$("$SHELL" "$type_nameref")
	[[ $exp == "$got" ]] || err_exit "Compound variable \$context is not printed with 'print -v'." \
		$'Diff follows:\n'"$(diff -u <(print -r -- "$exp") <(print -r -- "$got") | sed $'s/^/\t| /')"
fi

# ======

# https://github.com/ksh93/ksh/issues/577
# https://github.com/ksh93/ksh/pull/581
[[ "[a] b [c] d" =~ ^\[[^]]+\] ]]
[[ ${.sh.match} == '[a]' ]] || err_exit 'pattern ^\[[^]]+ broken'

# ======
exit $((Errors<125?Errors:125))
