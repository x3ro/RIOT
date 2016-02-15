#!/usr/bin/env python3

import argparse
import sys

from string import Template

class ShellColors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    END = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def create_argument_parser():
    parser = argparse.ArgumentParser(description='Generate configuration struct and initialization code for the FTL storage layer.')

    parser.add_argument('--page-size', action='store', type=int, required=True,
                        help='Page size of the targeted flash device in bytes')

    parser.add_argument('--pages-per-block', action='store', type=int, required=True,
                        help='TODO')

    parser.add_argument('--total-blocks', action='store', type=int, required=True,
                        help='TODO')

    parser.add_argument('--ecc-size', action='store', type=int, required=True,
                        help='TODO')

    parser.add_argument('--number-of-subpages', action='store', type=int, required=True,
                        help='TODO')

    parser.add_argument('--partition-name', action='append', type=str,
                        help='TODO')

    parser.add_argument('--partition-size', action='append', type=str,
                        help='TODO')

    return parser



def validate_arguments(args):
    errors = []
    warnings = []

    # TODO evaluate if this is a good limit
    if args.page_size < 64:
        errors.append("Page sizes smaller than 64 bytes are not currently supported")

    if args.pages_per_block < 1:
        errors.append("Please specify a valid amount of pages per block")

    if args.total_blocks < 1:
        errors.append("Please specify a valid amount of total blocks")

    if args.ecc_size < 1:
        errors.append("ECC size too small")

    if args.page_size % args.number_of_subpages > 0:
        errors.append("Page size must be evenly divisible by the number of subpages")

    if len(args.partition_name) != len(args.partition_size):
        errors.append("The specified number of partition names must match the number of partition sizes")

    # TODO: Add check for sum of partition sizes


    if len(warnings) > 0:
        print(ShellColors.WARNING + "\nThe following configuration errors were found:\n")
        for warning in warnings:
            print("\t* " + error)
        print(ShellColors.END)

    if len(errors) == 0:
        return

    print(ShellColors.FAIL + "\nThe following configuration errors were found:\n")
    for error in errors:
        print("\t* " + error)
    print(ShellColors.END)

    sys.exit(1)


parser = create_argument_parser()
args = parser.parse_args()
validate_arguments(args)




template_file = open('ftl_config_template.c', 'r')
template = template_file.read()
t = Template(template)

partition_struct = ""
partition_init = ""
base_offset = 0
for partition in zip(args.partition_name, args.partition_size):
    size = 0
    if partition[1] == "remaining":
        size = args.total_blocks - base_offset - 1
    else:
        size = int(partition[1])

    partition_struct += "    ftl_partition_s {0}_partition;\n".format(partition[0])
    partition_init += """
        .{0}_partition = {{
            .base_offset = {1},
            .size = {2},
        }},\n\n""".format(partition[0], base_offset, size)

    base_offset += size

replacements = {
    'page_size': args.page_size,
    'subpage_size': int(args.page_size / args.number_of_subpages),
    'pages_per_block': args.pages_per_block,
    'total_pages': int(args.total_blocks * args.pages_per_block),
    'ecc_size': args.ecc_size,

    'partition_init': partition_init,
    'partition_struct': partition_struct
}

print(t.substitute(replacements))


# ./ftl_config.py --page-size 512 --pages-per-block 1024 --total-blocks 16 \
#            --ecc-size 6 --number-of-subpages 4 \
#            --partition-name "index" --partition-size 3 \
#            --partition-name "firmware" --partition-size 2 \
#            --partition-name "sensordata" --partition-size "remaining"
