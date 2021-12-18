./bin/ypathtrace --scene tests/01_surface/surface.json --output out/lowres/01_surface_720_256.jpg --shader pathtrace  --samples 256 --resolution 720
./bin/ypathtrace --scene tests/02_rollingteapot/rollingteapot.json --output out/lowres/02_rollingteapot_720_256.jpg --shader pathtrace  --samples 256 --resolution 720
./bin/ypathtrace --scene tests/03_volume/volume.json --output out/lowres/03_volume_720_256.jpg --shader volpathtrace  --samples 256 --resolution 720 --bounces 64
./bin/ypathtrace --scene tests/04_head1/head1.json --output out/lowres/04_head1_720_256.jpg --shader volpathtrace  --samples 256 --resolution 720
./bin/ypathtrace --scene tests/05_head1ss/head1ss.json --output out/lowres/05_head1ss_720_256.jpg --shader volpathtrace  --samples 256 --resolution 720 --bounces 64
