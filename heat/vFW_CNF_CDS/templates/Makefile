.PHONY: all clean helm base cba clean_all
CSAR_NAME ?= vfw_k8s_demo
CBA_NAME ?= vFW_CDS_CNF

clean_all:
	$(MAKE) clean
	$(MAKE) all

all: csar/ helm base cba
	#Can't use .csar extension or SDC will panic
	cd csar/ && zip -r $(CSAR_NAME).zip .
	mv csar/$(CSAR_NAME).zip .

helm: csar/
	make -C helm
	mv helm/*.tgz csar/

base: csar/
	cp base/* csar/

cba: csar/
	cd cba/ && zip -r $(CBA_NAME).zip .
	mv cba/$(CBA_NAME).zip csar/

csar/:
	mkdir csar/

clean:
	rm -rf csar/
	rm -f $(CSAR_NAME).zip
