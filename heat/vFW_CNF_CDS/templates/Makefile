.PHONY: all clean helm base cba


all: csar/ helm base cba
	#Can't use .csar extension or SDC will panic
	cd csar/ && zip -r vfw_k8s_demo.zip .
	mv csar/vfw_k8s_demo.zip .

helm: csar/
	make -C helm
	mv helm/*.tgz csar/

base: csar/
	cp base/* csar/

cba: csar/
	cd cba/ && zip -r vFW_CDS_CNF.zip .
	mv cba/vFW_CDS_CNF.zip csar/

csar/:
	mkdir csar/

clean:
	rm -rf csar/
	rm -f vfw_k8s_demo.zip
