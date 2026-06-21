from . import convert
from . import decompile
from . import compare
from . import IGI1_create
from . import IGI1_convert
from . import IGI1_decompile

def register_parser(cli):
	cmd = cli.add_parser('iff')
	sub = cmd.add_subparsers()
	sub.require = True

	sub_decompile = sub.add_parser('decompile')
	sub_decompile.add_argument('src', help="Source folder")
	sub_decompile.add_argument('dst', help="Destination folder")
	sub_decompile.set_defaults(func=decompile.func)

	sub_convert = sub.add_parser('convert')
	sub_convert.add_argument('src', help="Source folder")
	sub_convert.add_argument('dst', help="Destination folder")
	sub_convert.set_defaults(func=convert.func)	

	sub_compare = sub.add_parser('compare')
	sub_compare.add_argument('src', help="Source folder")
	sub_compare.add_argument('dst', help="Destination folder")
	sub_compare.set_defaults(func=compare.func)

	sub_IGI1_create = sub.add_parser('IGI1_create')
	sub_IGI1_create.add_argument('src', help="Source folder")
	sub_IGI1_create.add_argument('dst', help="Destination folder")
	sub_IGI1_create.set_defaults(func=IGI1_create.func)

	sub_IGI1_decompile = sub.add_parser('IGI1_decompile')
	sub_IGI1_decompile.add_argument('src', help="Source folder")
	sub_IGI1_decompile.add_argument('dst', help="Destination folder")
	sub_IGI1_decompile.set_defaults(func=IGI1_decompile.func)

	sub_IGI1_convert = sub.add_parser('IGI1_convert')
	sub_IGI1_convert.add_argument('src', help="Source folder")
	sub_IGI1_convert.add_argument('dst', help="Destination folder")
	sub_IGI1_convert.set_defaults(func=IGI1_convert.func)
