#!/usr/bin/env sh

set -euxo pipefail

cd install-arbor
./install-local.sh
cd -

cd test/unit/inputs
rm -f *.h5
/usr/bin/env python3 generate/gen_edges.py
/usr/bin/env python3 generate/gen_nodes.py
/usr/bin/env python3 generate/gen_spikes.py
cd -

cd example/inputs
rm -f *.h5
/usr/bin/env python3 generate/gen_spikes.py
cd -

cd example/network
rm -f *.h5
/usr/bin/env python3 generate/gen_edges.py
/usr/bin/env python3 generate/gen_nodes.py
cd -
