set -eux

echo $1
echo $2

python ~/r500/regs/us_disassemble.py $1 > /run/user/$UID/$(basename $1)
python ~/r500/regs/us_disassemble.py $2 > /run/user/$UID/$(basename $2)

diff --color=always -u /run/user/$UID/$(basename $1) /run/user/$UID/$(basename $2)
