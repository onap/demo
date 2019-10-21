.PHONY: all clean helm base


all: csar/ helm base
	cd csar/ && zip -r vfw_k8s_demo.zip .
	mv csar/vfw_k8s_demo.zip .

helm: csar/
	make -C helm
	cp helm/*.tgz csar/

base: csar/
	cp base/* csar/

csar/:
	mkdir csar/

clean:
	rm -rf csar/
	rm -f vfw_k8s_demo.zip
