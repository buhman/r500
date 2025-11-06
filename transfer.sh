set -eux
make -C $HOME/r500/src shaders
#printf @`date +%s` | ssh sony xargs date -s
rsync -arv --checksum --ignore-times --no-times $HOME/r500 sony:/root
#rsync -arv --delete --checksum --ignore-times --no-times $HOME/r500 sony:/root
