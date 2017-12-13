#!/bin/bash


ctags -R --c++-kinds=+p --fields=+iaS --extra=+q
cscope -Rbq

echo -e "Creat ctags && cscope"
exit 0






