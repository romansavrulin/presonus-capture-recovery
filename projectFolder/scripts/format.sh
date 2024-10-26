#!/bin/bash

exit 0

find projectFolder/src/ -iname *.h -o -iname *.cpp | xargs clang-format -i

find projectFolder/tests/ -iname *.h -o -iname *.cpp | xargs clang-format -i

# Check if any changes were made by clang-format
if git diff --exit-code; then
  echo "Formatting is ok, no changes applied"
else
  echo "Formatting is wrong, changes have been applied"
  exit 1
fi
