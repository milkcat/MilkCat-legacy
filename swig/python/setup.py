#!/usr/bin/env python

from distutils.core import setup, Extension, os

setup(name = "MilkCat",
      py_modules = ["milkcat_raw", "milkcat"],
      ext_modules = [Extension("_milkcat_raw", ["milkcat_wrap.c",], libraries=["milkcat"])])
