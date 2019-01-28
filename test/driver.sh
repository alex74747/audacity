#!/bin/sh

##### Test driver script, to be invoked from the script's directory

##### usage

if (( $# != 1 )); then
    echo "Usage: $0 <path to Audacity executable>"
    exit
fi

##### variables

# How many seconds to allow for each test before killing it?
timeout=300

# Where are config files stored?
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    datadir=~/.audacity-data/
elif [[ "$OSTYPE" == "darwin"* ]]; then
    datadir=~/Library/Application\ Support/audacity/
elif [[ "$OSTYPE" == "cygwin" ]]; then
    datadir=~/AppData/Roaming/audacity/
fi

# Which config files must be backed up?
savefiles="audacity.cfg pluginregistry.cfg pluginsettings.cfg"

# Our scratch directory
tmpdir=${TMPDIR}audacity-tests/

# Where to collect coverage points from source code
expected=${tmpdir}expected.txt

# Where to collect coverage information from the test run
covered=${tmpdir}covered.txt

# Where to write results
results=RESULTS.txt

##### functions

# how to backup configuration files
save()
{
    mkdir $tmpdir
    for file in $savefiles; do cp "${datadir}${file}" "${tmpdir}"; done
}

# how to restore configuration files and exit
restore()
{
    for file in $savefiles; do mv "${tmpdir}${file}" "${datadir}"; done
    rm -rf "${tmpdir}"
    exit
}

# how to set up for one test
# argument is the individual test's directory
init()
{
    # make initial state of config files
    for file in ${savefiles}; do
	rm "${datadir}${file}";
	if [ -e "$1/${file}" ]; then cp "$1/${file}" "${datadir}"; fi
    done

    # copy other data files
    if [ -e "$1/files" ]; then cp -R "$1/files" "${datadir}"; fi
}

##### entry

# do the save, and restore in case of Ctrl-C
save
trap restore SIGINT

# scan the source code tree for code coverage points
find ../src -name '*.cpp' -o -name '*.h' -o -name '*.mm' |
    grep -v "/Journal.h$" |
    sort |
    xargs grep -n JOURNAL_COVERAGE |
    sed -E 's#../src/([^:]*):([^:]*):.*#\1 \2#' >
"${expected}"

# do the tests.  Each is in its own folder
for dir in `find * -type d`; do
    
    # folder should have a journal in it
    test=${dir}/journal.txt
    if [ ! -e "${test}" ] ; then continue ; fi

    # setup files
    init "${dir}"

    # initial time for timeout test
    start=`date +%s`
  
    # spawn the command, also getting the process ID so we can kill it
    # if it goes too long.  (Is there an easier way?)
    cmd="$1 -j \"`pwd`/${test}\"" # passes path to Audacity on command line
    sh -c "echo \$\$; exec ${cmd}" 2>&1 |
	{
	    read pid
	    line=
	    while [[ "$line" != "Journal done" ]] ; do
		# kill process if it runs away
		if (( `date +%s` - $start >= $timeout )); then
		    kill $pid;
		    break;
		fi
		# try to get a line of output, but don't wait too long
		if read -t 5 line; then
		    if [[ "$line" =~ ^coverage ]] ; then
			echo ${line#"coverage "} >> "${covered}"
		    elif [[ "$line" =~ "Journal failed at line " ]] ; then
			line=${line#"Journal failed at line "}
		    fi
		fi
	    done
	}
    if [[ "$line" == "Journal done" ]] ; then
	echo "${dir} succeeded"
    else
	echo "${dir} failed at ${line}"
    fi
done | tee "${results}"

# summarize coverage
echo >> "${results}"
echo "Missed coverage points:" >> "${results}"
sort --key=2n "${tmpdir}covered.txt" | # minor sort by line
    sort -s --key=1 | # major sort by name
    uniq |
    comm -13 - "${tmpdir}expected.txt" >>
"${results}"

echo "Output saved in ${results}"

# done
restore
