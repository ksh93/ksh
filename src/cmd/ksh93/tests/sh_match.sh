########################################################################
#                                                                      #
#               This software is part of the ast package               #
#          Copyright (c) 1982-2012 AT&T Intellectual Property          #
#                    Copyright (c) 2012 Roland Mainz                   #
#          Copyright (c) 2020-2024 Contributors to ksh 93u+m           #
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
for c in '!' '"' '#' '$' '%' '&' \' '(' ')' '*' '+' ',' '-' '.' '/' ':' ';' \
		'<' '=' '>' '?' '@' '[' '\\' ']' '^' '_' '`' '{' '|' '}' '~'
do	[[ $c =~ [[:punct:]] ]] || err_exit "pattern [[:punct:]] broken for $c"
done
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

# Avoid printing excessive elements for .sh.match
# https://github.com/ksh93/ksh/issues/308#issuecomment-1033259414
# https://github.com/ksh93/ksh/pull/709
exp='.sh.match .sh.match[1] .sh.match[2]'
got=${ $SHELL -c 'print ${!.sh.match} ${!.sh.match[1]} ${!.sh.match[2]}' }
[[ $exp == "$got" ]] || err_exit "'print \${!.sh.match}' should not print excessive elements" \
	"(expected ${ printf %q "$exp" }, got ${ printf %q "$got" })"

# ======
# Test repetiton handling in the shell glob pattern and regular expression engine
# https://github.com/ksh93/ksh/issues/207

# Helper function and alias for the tests below.
# Given --glob, a ksh glob pattern is matched, otherwise an extended regular expression.
# Each argument after the pattern is the expected value of the corresponding submatch.
# The special argument UNSET indicates that the submatch should not be set by the match.
function _checkmatch  # <lineno> [ --glob ] <string> <pattern> [ <expected group 0> [ <expected group 1> ... ] ]
{
	typeset lineno=$1
	shift
	if	[[ $1 == --glob ]]
	then	typeset patop='=='
		shift
	else	typeset patop='=~'
	fi
	typeset -i i
	typeset str=$1 pat=$2 got
	shift 2
	exp=$*
	if	eval "[[ \$str $patop \$pat ]]"
	then	for ((i = 0; i < 7; i++))
		do	got+=${got+ }${.sh.match[i]-UNSET}
		done
	else	got=NOMATCH
	fi
	[[ $got == "$exp" ]] || err\_exit "$lineno" "[[ $str $patop $pat ]] produced incorrect (sub)matches" \
		"(expected $(printf %q "$exp"), got $(printf %q "$got"))"
}
alias checkmatch='_checkmatch "$LINENO"'

# All of the tests below already passed before #207 was fixed. The tests
# were designed to add to our regression test converage in the area of
# repetitions and to verify that the recursion-avoiding rewrite of the
# repetition handling code doesn't introduce regressions.

# Set 1: basic tests for the regular expressions engine
# (extended regular expressions via [[ string =~ ERE ]]).

# 1. back-reference state across repetition iterations
checkmatch abbabb '(a(b)\2)+' abbabb abb b UNSET UNSET UNSET UNSET
checkmatch abbabb '(a(b)\2){2}' abbabb abb b UNSET UNSET UNSET UNSET
checkmatch abcabc '(a(bc)\2)+' NOMATCH
checkmatch aabbaabb '(a(b)\2|b)+' abb abb b UNSET UNSET UNSET UNSET
checkmatch xy '(x)\1' NOMATCH
checkmatch xyz '(x)(y)\1\2' NOMATCH
checkmatch ababab '(a|b)*' ababab b UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '((a)\2)+' aaaa aa a UNSET UNSET UNSET UNSET
checkmatch abbb '(a(bb)\2)+' NOMATCH
checkmatch abcabcabc '((a)(bc)\2)+' abca abca a bc UNSET UNSET UNSET
checkmatch aabb '(a(b)\2)?b' b UNSET UNSET UNSET UNSET UNSET UNSET

# 2. greedy / lazy / minimal
checkmatch aaaa 'a*' aaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa 'a*?' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa a+ aaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa 'a+?' a UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa 'a{2,3}' aaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa 'a{2,3}?' aa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaa 'a{2,}?' aa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch baaaab 'a*' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '(abc)+' abcabc abc UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '(abc)+?' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch xyz 'x*' x UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch '' 'a*' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch '' '(a*)*' '' '' UNSET UNSET UNSET UNSET UNSET
checkmatch '' '(a*)+' '' '' UNSET UNSET UNSET UNSET UNSET
checkmatch '' '(a?)*' '' '' UNSET UNSET UNSET UNSET UNSET
checkmatch aaa 'a{3}' aaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa 'a{3}?' aaa UNSET UNSET UNSET UNSET UNSET UNSET

# 3. empty-iteration / zero-width loops
checkmatch ab '(a|)*' a a UNSET UNSET UNSET UNSET UNSET
checkmatch ab '(a|b)*' ab b UNSET UNSET UNSET UNSET UNSET
checkmatch aa '(a*)*' aa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aa '(a*)+' aa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aaa '(a?)*' aaa a UNSET UNSET UNSET UNSET UNSET
checkmatch b '(a*)*b' b '' UNSET UNSET UNSET UNSET UNSET
checkmatch aab '(|a)*b' aab a UNSET UNSET UNSET UNSET UNSET
checkmatch '' '(|a)*' '' '' UNSET UNSET UNSET UNSET UNSET
checkmatch abab '(a|ab)*' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch aba '(a|ab)*' aba a UNSET UNSET UNSET UNSET UNSET
checkmatch ab '(a|b)*?' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aa 'a**' aa UNSET UNSET UNSET UNSET UNSET UNSET

# 4. alternation / groups
checkmatch abc 'a(b|c)d' NOMATCH
checkmatch abd 'a(b|c)d' abd b UNSET UNSET UNSET UNSET UNSET
checkmatch xyz '(x)(y)(z)' xyz x y z UNSET UNSET UNSET
checkmatch abcabc '(abc|abd)' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch abd '(abc|abd)' abd abd UNSET UNSET UNSET UNSET UNSET
checkmatch aaa '(a|aa)+' aaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaa '(a|aa)+' aaaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa '(a|aa)*' aaaaaa aa UNSET UNSET UNSET UNSET UNSET
checkmatch x '(a)?x' x UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch x '(a)*x' x UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc 'a(b(c))d' NOMATCH

# 5. case folding
checkmatch ABC '(abc)' NOMATCH
checkmatch ABC '(abc)' NOMATCH
checkmatch ABCabc '(abc)+' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch ABC '[[:upper:]]+' ABC UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc '[[:lower:]]+' abc UNSET UNSET UNSET UNSET UNSET UNSET

# 6. anchors, word boundaries
checkmatch xabc ^abc NOMATCH
checkmatch xabc 'abc$' abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^abc$' abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch a-b '\b-\b' - UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc '\<abc\>' abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch ab '\B' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch 'a b' '\s\w+' ' b' UNSET UNSET UNSET UNSET UNSET UNSET

# 7. dot / classes / collation
checkmatch abc a.c abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc '[^x]+' abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc '[[:alpha:]][[:alnum:]][[:alpha:]]' abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch a-b '[[:punct:]]' - UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch a1 '\w\d' a1 UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch xyz '[[=x=]][[=y=]][[=z=]]' xyz UNSET UNSET UNSET UNSET UNSET UNSET

# 8. back-references
checkmatch abab '(ab)\1' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab)\1\1' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '(a)(a)\1\2' aaaa a a UNSET UNSET UNSET UNSET
checkmatch abcabc '([a-c])\1' NOMATCH
checkmatch xyzxyz '(xyz)\1*' xyzxyz xyz UNSET UNSET UNSET UNSET UNSET
checkmatch ab '(a)?b' ab a UNSET UNSET UNSET UNSET UNSET
checkmatch b '(a)?b' b UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '(a+)\1' aaaa aa UNSET UNSET UNSET UNSET UNSET

# 9. nested repetitions
checkmatch aaaa '((a)*)*' aaaa aaaa a UNSET UNSET UNSET UNSET
checkmatch aaaaaa '((a*)*)*' aaaaaa aaaaaa aaaaaa UNSET UNSET UNSET UNSET
checkmatch ababab '((ab)*)*' ababab ababab ab UNSET UNSET UNSET UNSET
checkmatch abcabc '((abc)*)*' abcabc abcabc abc UNSET UNSET UNSET UNSET
checkmatch aaa '(a*(a*)*)*' aaa aaa '' UNSET UNSET UNSET UNSET
checkmatch abab '((a)(b))*' abab ab a b UNSET UNSET UNSET
checkmatch abcabcabc '((abc)*)*' abcabcabc abcabcabc abc UNSET UNSET UNSET UNSET
checkmatch aaaaaa '(a**)*' aaaaaa aaaaaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa '((a)|(aa))*' aaaaaa aa UNSET aa UNSET UNSET UNSET

# 10. cut / negation interactions
checkmatch abc '(~a)b' NOMATCH
checkmatch abc '(~(a)b)c' NOMATCH
checkmatch abcd '(~(a|b))c' NOMATCH
checkmatch xyz '[^a]*' xyz UNSET UNSET UNSET UNSET UNSET UNSET

# 11. longer repetitions
checkmatch aaaaaaaaaaaaaaaaaaaaaaaaaaaa a+ aaaaaaaaaaaaaaaaaaaaaaaaaaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abababababababababababababab '(ab)+' abababababababababababababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abababababababababababababab '(ab)*' abababababababababababababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abababababababababababababab '(a|b)+' abababababababababababababab b UNSET UNSET UNSET UNSET UNSET
checkmatch abababababababababababababab '(a*)(b*)' ab a b UNSET UNSET UNSET UNSET
checkmatch abcdefabcdefabcdef '(abcdef)+' abcdefabcdefabcdef abcdef UNSET UNSET UNSET UNSET UNSET
checkmatch abcdefabcdefabcdef '(abcdef)*' abcdefabcdefabcdef abcdef UNSET UNSET UNSET UNSET UNSET
checkmatch xyxyxyxyxyxy '(xy)+' xyxyxyxyxyxy xy UNSET UNSET UNSET UNSET UNSET
checkmatch a1a1a1a1 '([a-z][0-9])+' a1a1a1a1 a1 UNSET UNSET UNSET UNSET UNSET
checkmatch ab_cd_ab_cd '(ab_cd)+' ab_cd ab_cd UNSET UNSET UNSET UNSET UNSET
checkmatch xy '(x|y)+' xy y UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa '(a|aa|aaa)+' aaaaaa aaa UNSET UNSET UNSET UNSET UNSET
checkmatch aXbXcX '(a|[A-Z])X' aX a UNSET UNSET UNSET UNSET UNSET
checkmatch abcdabcd '(abcd){2}' abcdabcd abcd UNSET UNSET UNSET UNSET UNSET

# 12. minimal/greedy with trailing continuation
checkmatch aaa 'a*bc' NOMATCH
checkmatch aaab 'a*ab' aaab UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '(a|ab)c' abc ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(a|ab)*c' NOMATCH
checkmatch aaaa 'a*a' aaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa 'a*aa' aaaaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abab 'a*b' ab UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aab 'a?ab' aab UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaab 'a??ab' aab UNSET UNSET UNSET UNSET UNSET UNSET

# Set 2: ambiguity and backtracking inside repetitions, where the
# engine's choice between equally long matches is decided by better().

# alternation inside a repetition, differing alternative lengths
checkmatch aaaaaa '^(a|aa)*$' aaaaaa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa '^((a)|(aa))*$' aaaaaa aa UNSET aa UNSET UNSET UNSET
checkmatch aaaaaa '^((a)|(aa))+$' aaaaaa aa UNSET aa UNSET UNSET UNSET
checkmatch aaaaaa '^(aa|a)*$' aaaaaa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|aa)*$' aaaa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aaa '^(a|aa)*$' aaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa '^(a|aa|aaa)*$' aaaaaa aaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaaa '^((a|aa))*$' aaaaaa aa aa UNSET UNSET UNSET UNSET
checkmatch aaaaaa '^(a|aaa|aa)*$' aaaaaa aaa UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '^(a|ab|c)*$' abcabc c UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '^((a|ab|c))*$' abcabc c c UNSET UNSET UNSET UNSET
checkmatch aababa '^(a|ab)*$' aababa a UNSET UNSET UNSET UNSET UNSET
checkmatch aabab '^(a|ab)*b$' aabab a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|aa)+$' aaaa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aab '^(a|ab)+$' aab ab UNSET UNSET UNSET UNSET UNSET
checkmatch xaaay '^(x|a)*y$' xaaay a UNSET UNSET UNSET UNSET UNSET

# ambiguous repetition boundaries
checkmatch aaaa '^(a*)(a*)$' aaaa aaaa '' UNSET UNSET UNSET UNSET
checkmatch aaaa '^((a*)(a*))$' aaaa aaaa aaaa '' UNSET UNSET UNSET
checkmatch aaaa '^(a*)(a*)(a*)$' aaaa aaaa '' '' UNSET UNSET UNSET
checkmatch aaab '^(a*)(b)$' aaab aaa b UNSET UNSET UNSET UNSET
checkmatch aaaab '^(a*)(a*)(b)$' aaaab aaaa '' b UNSET UNSET UNSET
checkmatch abab '^((ab)*|(ba)*)$' abab abab ab UNSET UNSET UNSET UNSET
checkmatch ababab '^((ab)*|ab)*$' ababab ababab ab UNSET UNSET UNSET UNSET
checkmatch aaaa '^(aa|a)+$' aaaa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^((aa|a))+a$' aaaa a a UNSET UNSET UNSET UNSET
checkmatch abcabc '^((abc)|(ab|c))*abc$' abcabc abc abc UNSET UNSET UNSET UNSET

# repetition body with choice ending at different positions
checkmatch ababab '^((a)(b*))+$' ababab ab a b UNSET UNSET UNSET
checkmatch ababab '^((a)|(b*))+$' ababab b UNSET b UNSET UNSET UNSET
checkmatch abcabc '^((ab)|(c))*$' abcabc c UNSET c UNSET UNSET UNSET
checkmatch xabab '^x((a)|(ab))*$' xabab ab UNSET ab UNSET UNSET UNSET

# nested repetitions
checkmatch aaaa '^((a)*)*$' aaaa aaaa a UNSET UNSET UNSET UNSET
checkmatch aaaa '^((a*)*)*$' aaaa aaaa aaaa UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a**(a*)*)*$' aaaa aaaa '' UNSET UNSET UNSET UNSET
checkmatch aaa '^((a)?)*$' aaa a a UNSET UNSET UNSET UNSET
checkmatch aaaa '^((a)+)*$' aaaa aaaa a UNSET UNSET UNSET UNSET
checkmatch ababab '^((ab)*)*$' ababab ababab ab UNSET UNSET UNSET UNSET
checkmatch aaaaaa '^((a)|(aa))*((a)|(aa))$' aaaaaa a a UNSET a a UNSET

# back-references across iterations
checkmatch abbabb '^((a)(b)\2)+$' NOMATCH
checkmatch aabbaa '^((a)(b)\2)*$' NOMATCH
checkmatch aaaaaa '^((a)\1)+$' NOMATCH
checkmatch abcabc '^((abc)\1)*$' NOMATCH
checkmatch abab '^((a)(b)\1\2)$' NOMATCH
checkmatch aaa '^((a)\1?)*$' NOMATCH

# anchors and alternation together
checkmatch aaaa '^(a|aa)*a$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^((a)|(aa))*aa$' aaaa aa UNSET aa UNSET UNSET UNSET
checkmatch baaa '^([ab]|a)*$' baaa a UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^(a|b|ab)*c$' abc ab UNSET UNSET UNSET UNSET UNSET
checkmatch cab '^(a|b|ab)*c$' NOMATCH

# Set 3: alternation with possibly empty branches inside a repetition.

# an alternative that can match the empty string inside a repetition
checkmatch aaaa '^(a|b*)$' NOMATCH
checkmatch a '^(a|b*)$' a a UNSET UNSET UNSET UNSET UNSET
checkmatch ab '^(a|b*)$' NOMATCH
checkmatch aabb '^(a|b*)$' NOMATCH
checkmatch b '^(a|b*)$' b b UNSET UNSET UNSET UNSET UNSET
checkmatch '' '^(a|b*)$' '' '' UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*)c*$' NOMATCH
checkmatch aaaac '^(a|b*)c*$' NOMATCH
checkmatch aaa '^(b*a)*$' aaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aab '^(b*a)*$' NOMATCH
checkmatch aba '^(b*a)*$' aba ba UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a*|b)*$' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b|a*)*$' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a*|b*)*$' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*)+$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*){2,}$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*)?$' NOMATCH
checkmatch aaaa '^(a|b*){3}$' NOMATCH
checkmatch aaaa '^(a+|b*)$' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a*|b+)$' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*)(a|b*)$' NOMATCH
checkmatch aaaa '^(a|b*)*a$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*)*$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|(b*))*c$' NOMATCH
checkmatch aaab '^(a|(b*))*b$' aaab a UNSET UNSET UNSET UNSET UNSET
checkmatch aabb '^(a|(b*))*bb$' aabb a UNSET UNSET UNSET UNSET UNSET
checkmatch aa '^((a)|(b*))*$' aa a a UNSET UNSET UNSET UNSET
checkmatch aa '^((a)|(b*))+$' aa a a UNSET UNSET UNSET UNSET
checkmatch ab '^(a|b)(a|b)*$' ab a b UNSET UNSET UNSET UNSET
checkmatch aba '^(a|b)(a|b)*$' aba a a UNSET UNSET UNSET UNSET
checkmatch ab '^(a?b?)*$' ab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ab '^(a?b?)+$' ab ab UNSET UNSET UNSET UNSET UNSET
checkmatch aa '^(a?|b?)*$' aa a UNSET UNSET UNSET UNSET UNSET
checkmatch abab '^(a|b?)*$' abab b UNSET UNSET UNSET UNSET UNSET
checkmatch '' '^(a?)*$' '' '' UNSET UNSET UNSET UNSET UNSET
checkmatch aa '^(a?)*b$' NOMATCH

# same shapes, minimal repetition
checkmatch aaaa '^(a|b*)*?$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*)+?$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|b*){2,}?$' aaaa a UNSET UNSET UNSET UNSET UNSET

# ksh glob pattern syntax uses the same engine
checkmatch --glob aaaa '@(a|b*)' NOMATCH
checkmatch --glob aaaa '*(a|b*)' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaaa '+(a|b*)' aaaa aaaa UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaaa '?(a|b*)' NOMATCH
checkmatch --glob aaaa '{2,}(a|b*)' aaaa aaaa UNSET UNSET UNSET UNSET UNSET

# Set 4: repetition bodies of fixed match length, i.e., bodies that reach
# their continuation at a unique position per iteration. These are handled
# by the fast parserep_fixedlen() function instead of the general but slow
# parserep(). This test set validates the semantics of that split.

# plain fixed length body, greedy and minimal
checkmatch ababab '(ab)+' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab)*' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababa '(ab)+' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababa '(ab)*' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ab '(ab)+' ab ab UNSET UNSET UNSET UNSET UNSET
checkmatch zzzzz '(ab)+' NOMATCH
checkmatch zababz '(ab)+' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch '' '(ab)*' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch '' '(ab)+' NOMATCH
checkmatch ababab '^(ab)+$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababx '^(ab)+$' NOMATCH
checkmatch abababababababababababx '^(ab)+x$' abababababababababababx ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababababababababababab '^(ab)+$' ababababababababababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab)+?' ab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab)*?' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab){2,3}' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab){2,3}?' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab){2,}' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '(ab){2}?' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abab '(ab){4}' NOMATCH
checkmatch ababab '(ab){0,2}' abab ab UNSET UNSET UNSET UNSET UNSET

# subexpressions inside the body: the submatch state per iteration
checkmatch ababab '^(ab)+$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '((ab))+$' ababab ab ab UNSET UNSET UNSET UNSET
checkmatch ababab '(((ab)))+$' ababab ab ab ab UNSET UNSET UNSET
checkmatch ababab '^(a(b))+$' ababab ab b UNSET UNSET UNSET UNSET
checkmatch ababab '^((a)(b))+$' ababab ab a b UNSET UNSET UNSET
checkmatch ababab '^(a(b)(c))+$' NOMATCH
checkmatch ababab '(ab)+x' NOMATCH
checkmatch zababz '(ab)+' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab 'x(ab)+' NOMATCH
checkmatch ababab '(ab)+(ab)+' ababab ab ab UNSET UNSET UNSET UNSET
checkmatch ababab '(ab)*(ab)*' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(a)(b)+$' NOMATCH
checkmatch a1b2c3 '^([a-z][0-9])+$' a1b2c3 c3 UNSET UNSET UNSET UNSET UNSET
checkmatch ab_cd_ab_cd '^(ab_cd)+$' NOMATCH
checkmatch ababab '^((ab)*)+$' ababab ababab ab UNSET UNSET UNSET UNSET
checkmatch ababab '^((ab)+)*$' ababab ababab ab UNSET UNSET UNSET UNSET
checkmatch aaaa '^((a)*)*$' aaaa aaaa a UNSET UNSET UNSET UNSET
checkmatch aaaa '^((a)*)+$' aaaa aaaa a UNSET UNSET UNSET UNSET
checkmatch aaaa '^(((a)))+$' aaaa a a a UNSET UNSET UNSET
checkmatch aaa '^(a+)+$' aaa aaa UNSET UNSET UNSET UNSET UNSET
checkmatch abab '^(ab)*a$' NOMATCH

# one character and character class bodies are fixed length too
checkmatch abc '^(.)$' NOMATCH
checkmatch abc '^(.)+$' abc c UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^(..)+$' NOMATCH
checkmatch abc '(.+)' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^(.+)(.+)$' abc ab c UNSET UNSET UNSET UNSET
checkmatch abc '^(.+?)(.+?)$' abc a bc UNSET UNSET UNSET UNSET
checkmatch aabb '^[ab]+$' aabb UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aabb '^[^a]+$' NOMATCH
checkmatch abc '^[a-c]+$' abc UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaa '^a{3}$' aaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^a{2,4}$' aaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^a{2,4}?$' aaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaaaa '^a{2,}$' aaaaa UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch aaa '^(a){3}$' aaa a UNSET UNSET UNSET UNSET UNSET
checkmatch abab '^(ab){2}$' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab){1,2}$' NOMATCH

# alternatives whose branches all have the same length
checkmatch abab '^(ab|ba)+$' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abba '^(ab|ba)+$' abba ba UNSET UNSET UNSET UNSET UNSET
checkmatch abab '^(a|b)+$' abab b UNSET UNSET UNSET UNSET UNSET
checkmatch abcd '^(ab|cd)+$' abcd cd UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^(abc|xyz)+$' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab|ba|cd)+$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch aaaa '^(a|aa|aaa)+$' aaaa a UNSET UNSET UNSET UNSET UNSET
checkmatch ab '^(ab|a)+$' ab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abab '^(ab|a)+$' abab ab UNSET UNSET UNSET UNSET UNSET

# negated bodies are not fixed length; they use the slow general matcher
checkmatch abc '^(~a)b$' NOMATCH
checkmatch abc '^(~(a)b)c$' NOMATCH
checkmatch xyz '^[^a]*$' xyz UNSET UNSET UNSET UNSET UNSET UNSET

# back-references are not fixed length either
checkmatch abbabb '^(a(b)\2)+$' abbabb abb b UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab)\1$' NOMATCH

# bodies that cannot match often enough, or stop short
checkmatch abc '^(ab){2,}$' NOMATCH
checkmatch ab '^(ab){2,}$' NOMATCH
checkmatch ababab '^(ab)+c$' NOMATCH
checkmatch ababa '^(ab)+b$' NOMATCH
checkmatch aaa '^(aa)+$' NOMATCH
checkmatch aaaa '^(aa)+$' aaaa aa UNSET UNSET UNSET UNSET UNSET

# nested repetitions of fixed length bodies in shell pattern syntax
checkmatch --glob ababab '+(ab)' ababab ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob ababab '*(ab)' ababab ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob ababab '+(ab)*' ababab ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob ababab '*(ab)*' ababab ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaaaaaaa '+(aa)' aaaaaaaa aaaaaaaa UNSET UNSET UNSET UNSET UNSET
checkmatch --glob ababab '@(+(ab))' ababab ababab ababab UNSET UNSET UNSET UNSET
checkmatch --glob ababab '+(ab|ba)' ababab ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaa '+(a)' aaa aaa UNSET UNSET UNSET UNSET UNSET
checkmatch --glob abc '+(a|b|c)' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaaaaaaaaa '@(+(aa))' aaaaaaaaaa aaaaaaaaaa aaaaaaaaaa UNSET UNSET UNSET UNSET
checkmatch --glob ababab '+(a)(b)+' NOMATCH
checkmatch --glob xababab 'x+(ab)' xababab ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob abababx '+(ab)x' abababx ababab UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaaaaaaa '+(aa)b' NOMATCH
checkmatch --glob abc '+(~(a))b' NOMATCH
checkmatch --glob ababab '~(ab)*' NOMATCH
checkmatch --glob ababab '~(ab)+' NOMATCH

# Set 5: edge cases of repetition bodies whose match length is fixed, which
# parserep_fixedlen() handles: bodies that can match the empty string, zero
# and fixed dup counts, and the corresponding minimal variants.

# bodies that can match the empty string
checkmatch aaa '^(*)$' NOMATCH
checkmatch '' '^(*)$' NOMATCH
checkmatch aaa '^(*a)$' NOMATCH
checkmatch aa '^(a*)*$' aa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aa '^(a*)+$' aa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aaa '^(a{0})*$' NOMATCH
checkmatch aaa '^(a{0})+$' NOMATCH
checkmatch aaa '^(a{0}b)*$' NOMATCH
checkmatch aaa '^(a{0}b)+$' NOMATCH
checkmatch 'a$b' '^(^)*$' NOMATCH
checkmatch 'a$b' '^(a$)*$' NOMATCH

# zero and fixed dup counts
checkmatch ababab '^(ab){0}$' NOMATCH
checkmatch ababab '^(ab){0}x*$' NOMATCH
checkmatch ababab '^(ab){0,3}$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab){0,3}?$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab){3}$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab){4}$' NOMATCH
checkmatch ab '^(ab){2}$' NOMATCH
checkmatch a '^(ab){2}$' NOMATCH
checkmatch abab '^(a){2}$' NOMATCH
checkmatch a '^(a){0}$' NOMATCH
checkmatch '' '^(a){0}$' '' UNSET UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^(a|b|c){3}$' abc c UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '^(a|b|c){6}$' abcabc c UNSET UNSET UNSET UNSET UNSET
checkmatch abcd '^(ab|cd){2}$' abcd cd UNSET UNSET UNSET UNSET UNSET
checkmatch abc '^(abc){2}$' NOMATCH
checkmatch abcd '^(ab|cd){2,4}$' abcd cd UNSET UNSET UNSET UNSET UNSET
checkmatch abcd '^(ab|cd){2,4}?$' abcd cd UNSET UNSET UNSET UNSET UNSET
checkmatch abababab '^((ab){2})+$' abababab abab ab UNSET UNSET UNSET UNSET
checkmatch ababab '^((ab){2})b$' NOMATCH
checkmatch ababab '^((ab){2})+$' NOMATCH

# nested repetitions of fixed length bodies
checkmatch abababab '^((ab)+)+$' abababab abababab ab UNSET UNSET UNSET UNSET
checkmatch abababab '^(((ab)+)+)+$' abababab abababab abababab ab UNSET UNSET UNSET
checkmatch abababab '^(ab)+(ab)+$' abababab ab ab UNSET UNSET UNSET UNSET
checkmatch abababab '^((ab){2}(ab){2})$' abababab abababab ab ab UNSET UNSET UNSET
checkmatch ababab '^(ab){2}(ab)$' ababab ab ab UNSET UNSET UNSET UNSET

# minimal
checkmatch ababab '^(ab){2,}?$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab){2,3}?$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab)*?$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abcabc '^(a|b|c)*?$' abcabc c UNSET UNSET UNSET UNSET UNSET
checkmatch ababab '^(ab)+?$' ababab ab UNSET UNSET UNSET UNSET UNSET
checkmatch '' '^(ab)*?$' '' UNSET UNSET UNSET UNSET UNSET UNSET

# the same in ksh glob pattern syntax
checkmatch --glob aaa '*(a)' aaa aaa UNSET UNSET UNSET UNSET UNSET
checkmatch --glob aaa '*(a{0})' NOMATCH
checkmatch --glob ababab '*(ab){3}' NOMATCH
checkmatch --glob ababab '+(ab){0}' NOMATCH
checkmatch --glob ababab '?(ab)' NOMATCH
checkmatch --glob ababab '~(ab)+?' NOMATCH
checkmatch --glob ababab '@(ab){2}' NOMATCH
checkmatch --glob abc '+(a|b|c)' abc abc UNSET UNSET UNSET UNSET UNSET
checkmatch --glob abcabcabc '+(a|b|c)' abcabcabc abcabcabc UNSET UNSET UNSET UNSET UNSET
checkmatch --glob abababab '@(+((ab)+))' NOMATCH

# Set 6: a repetition body that can reach its continuation at more than one
# position per iteration, such as an alternation whose branches differ in
# length or a nested variable-length repetition, cannot be scanned one
# iteration at a time. Such bodies keep the slower general matcher, which
# ranks the alternatives with better(). These are regressions from an
# attempt that failed to use the general matcher in such cases.

checkmatch ab '(ab|a)+' ab ab UNSET UNSET UNSET UNSET UNSET
checkmatch abab '(ab|a)+' abab ab UNSET UNSET UNSET UNSET UNSET
checkmatch aa '(a*)+' aa aa UNSET UNSET UNSET UNSET UNSET
checkmatch aa '(a+)+' aa aa UNSET UNSET UNSET UNSET UNSET

unset -f _checkmatch
unalias checkmatch

# ======
exit $((Errors<125?Errors:125))
