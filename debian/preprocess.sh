#!/bin/bash

set -e

typeset tmpfile="$('printf' 'mkstemp(template)\n' | 'm4' '-D' "template=${TMPDIR:-/tmp}/preprocTMPXXXXXXXXXXXXXXXX")"

function cleanup () {
  if [ -n "${tmpfile}" ] && [ -f "${tmpfile}" ]; then
    'rm' '-fv' "${tmpfile}" >&'2'
  fi
}

trap 'cleanup' 'EXIT' '6' 'BUS' 'ILL' 'KILL' 'QUIT' 'TERM' 'INT' 'PIPE' 'ALRM' 'USR1' 'USR2'

function replace_kv () {
  typeset key="${1:?'No key passed to replace_kv (), this is illegal.'}"
  typeset value="${2:?'No value passed to replace_kv (), this is illegal.'}"

  'sed' '-i' '-e' "s#@@${key}@@#${value}#g" "${tmpfile}"
}

function main () {
  typeset in_file="${1:?'No input file passed to main (), this is illegal.'}"
  typeset out_file="${in_file%'.in'}"

  shift

  # Check if temporary file is set up correctly.
  if [ -z "${tmpfile}" ]; then
    printf 'No temporary file to write to available, erroring out.\n' >&'2'
    return '1'
  fi

  if ! [ -f "${tmpfile}" ]; then
    printf 'Temporary file to write to "%s" not a regular file, erorring out.\n' "${tmpfile}" >&'2'
    return '2'
  fi

  # Check if in_file ends on ".in".
  if [ "${in_file}" = "${out_file}" ]; then
    printf 'Input file "%s" does not end in ".in", this is unsupported.\n' "${in_file}" >&'2'
    return '3'
  fi

  # Key-value pair arguments handling.
  if [ '1' -gt "${#}" ]; then
    printf 'At least one key-value pair must be provided.\n' >&'2'
    return '4'
  fi

  # Copy file contents to temporary file.
  'cp' "${in_file}" "${tmpfile}"

  typeset kv_i=''
  for kv_i in "${@}"; do
    typeset key="${kv_i%%'='*}"
    typeset value="${kv_i#*'='}"

    if [ -z "${key}" ]; then
      printf 'Key-value pair "%s" is invalid since it contains no key.\n' "${kv_i}" >&'2'
      return '5'
    fi

    'replace_kv' "${key}" "${value}"
  done

  # Now that all key-value pairs have been replaced, run the real
  # preprocessor.
  # We'll assume that cwd is the top source directory. Any other method, like
  # autodetection, would be difficult to pull off portably.
  'debian/preprocessor.pl' "${tmpfile}"

  # And, eventually, replace the actual file.
  'cp' "${tmpfile}" "${out_file}"
}

'main' "${@}"
