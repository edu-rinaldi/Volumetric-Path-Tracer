./bin/ypathtrace --scene tests/01_surface/surface.json --output out/highres/01_surface_1280_1024.jpg --shader pathtrace --samples 1024 --resolution 1280
./bin/ypathtrace --scene tests/02_rollingteapot/rollingteapot.json --output out/highres/02_rollingteapot_1280_1024.jpg --shader pathtrace --samples 1024 --resolution 1280
./bin/ypathtrace --scene tests/03_volume/volume.json --output out/highres/03_volume_1280_1024.jpg --shader volpathtrace --samples 1024 --resolution 1280 --bounces 64
./bin/ypathtrace --scene tests/04_head1/head1.json --output out/highres/04_head1_1280_1024.jpg --shader volpathtrace --samples 1024 --resolution 1280
./bin/ypathtrace --scene tests/05_head1ss/head1ss.json --output out/highres/05_head1ss_1280_1024.jpg --shader volpathtrace --samples 4096 --resolution 1280 --bounces 64

