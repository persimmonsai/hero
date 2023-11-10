

all: snitch-app/hello_world.bin cva6-app/bringup

snitch-app/hello_world.bin:
	mkdir -p snitch-app/build
	cd snitch-app/build && cmake .. && make

snitch-offload-app/snitch.bin:
	mkdir -p snitch-offload-app/build
	cd snitch-offload-app/build && cmake .. && make

cva6-app/bringup:
	make -C cva6-app

deploy: snitch-app/hello_world.bin cva6-app/bringup
	mkdir -p deploy_standalone
	cp run.sh cva6-app/bringup cva6-app/file_to_offload.txt snitch-app/build/hello_world.bin deploy_standalone
	rsync -r deploy_standalone root@hero-vcu128-02.ee.ethz.ch:/root/

zip: snitch.zip
zip-offload: snitch-offload.zip

snitch-offload.zip: snitch-offload-app/snitch.bin cva6-app/bringup
	mkdir -p deploy_standalone
	cp run_offload.sh cva6-app/bringup cva6-app/file_to_offload.txt snitch-offload-app/build/snitch.bin deploy_standalone

	zip -r deploy_standalone/snitch.zip deploy_standalone/run_offload.sh deploy_standalone/bringup deploy_standalone/file_to_offload.txt deploy_standalone/snitch.bin


snitch.zip: snitch-app/hello_world.bin cva6-app/bringup
	mkdir -p deploy_standalone
	cp run.sh cva6-app/bringup cva6-app/file_to_offload.txt snitch-app/build/hello_world.bin deploy_standalone

	zip -r deploy_standalone/snitch.zip deploy_standalone/run.sh deploy_standalone/bringup deploy_standalone/file_to_offload.txt deploy_standalone/hello_world.bin

clean:
	make -C cva6-app clean
	rm -rf snitch-app/build snitch-offload-app/build deploy_standalone