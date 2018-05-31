#!/usr/bin/python

import csv
import os

def main():
    while True:
        line = input()
        args_table = csv.reader([line], delimiter=' ', quotechar='"')
        for args in args_table:
            file_path = args[0]
            file_name = args[1]
            mail_address = args[2]
            print("{0} => {1}".format(file_path, mail_address))
            os.system('uuencode "{0}" | mail -s "{1}" "{2}"'.format(file_path, file_name, mail_address))


if __name__ == "__main__":
    main()
