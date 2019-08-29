// +build !

// This file was autogenerated by openapi-gen. Do not edit it manually!

package v1alpha1

import (
	spec "github.com/go-openapi/spec"
	common "k8s.io/kube-openapi/pkg/common"
)

func GetOpenAPIDefinitions(ref common.ReferenceCallback) map[string]common.OpenAPIDefinition {
	return map[string]common.OpenAPIDefinition{
		"./pkg/apis/onap/v1alpha1.Datasource":              schema_pkg_apis_onap_v1alpha1_Datasource(ref),
		"./pkg/apis/onap/v1alpha1.GrafanaDataSource":       schema_pkg_apis_onap_v1alpha1_GrafanaDataSource(ref),
		"./pkg/apis/onap/v1alpha1.GrafanaDataSourceSpec":   schema_pkg_apis_onap_v1alpha1_GrafanaDataSourceSpec(ref),
		"./pkg/apis/onap/v1alpha1.GrafanaDataSourceStatus": schema_pkg_apis_onap_v1alpha1_GrafanaDataSourceStatus(ref),
	}
}

func schema_pkg_apis_onap_v1alpha1_Datasource(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "Datasource defines the fields in a GrafanaDataSource",
				Properties: map[string]spec.Schema{
					"name": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"type": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"url": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
					"isDefault": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"boolean"},
							Format: "",
						},
					},
					"access": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
				Required: []string{"name", "type"},
			},
		},
		Dependencies: []string{},
	}
}

func schema_pkg_apis_onap_v1alpha1_GrafanaDataSource(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "GrafanaDataSource is the Schema for the grafanadatasources API",
				Properties: map[string]spec.Schema{
					"kind": {
						SchemaProps: spec.SchemaProps{
							Description: "Kind is a string value representing the REST resource this object represents. Servers may infer this from the endpoint the client submits requests to. Cannot be updated. In CamelCase. More info: https://git.k8s.io/community/contributors/devel/api-conventions.md#types-kinds",
							Type:        []string{"string"},
							Format:      "",
						},
					},
					"apiVersion": {
						SchemaProps: spec.SchemaProps{
							Description: "APIVersion defines the versioned schema of this representation of an object. Servers should convert recognized schemas to the latest internal value, and may reject unrecognized values. More info: https://git.k8s.io/community/contributors/devel/api-conventions.md#resources",
							Type:        []string{"string"},
							Format:      "",
						},
					},
					"metadata": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("k8s.io/apimachinery/pkg/apis/meta/v1.ObjectMeta"),
						},
					},
					"spec": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/onap/v1alpha1.GrafanaDataSourceSpec"),
						},
					},
					"status": {
						SchemaProps: spec.SchemaProps{
							Ref: ref("./pkg/apis/onap/v1alpha1.GrafanaDataSourceStatus"),
						},
					},
				},
			},
		},
		Dependencies: []string{
			"./pkg/apis/onap/v1alpha1.GrafanaDataSourceSpec", "./pkg/apis/onap/v1alpha1.GrafanaDataSourceStatus", "k8s.io/apimachinery/pkg/apis/meta/v1.ObjectMeta"},
	}
}

func schema_pkg_apis_onap_v1alpha1_GrafanaDataSourceSpec(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "GrafanaDataSourceSpec defines the desired state of GrafanaDataSource",
				Properties: map[string]spec.Schema{
					"datasources": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Ref: ref("./pkg/apis/onap/v1alpha1.Datasource"),
									},
								},
							},
						},
					},
					"grafana": {
						SchemaProps: spec.SchemaProps{
							Type: []string{"object"},
							AdditionalProperties: &spec.SchemaOrBool{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
				},
				Required: []string{"datasources", "grafana"},
			},
		},
		Dependencies: []string{
			"./pkg/apis/onap/v1alpha1.Datasource"},
	}
}

func schema_pkg_apis_onap_v1alpha1_GrafanaDataSourceStatus(ref common.ReferenceCallback) common.OpenAPIDefinition {
	return common.OpenAPIDefinition{
		Schema: spec.Schema{
			SchemaProps: spec.SchemaProps{
				Description: "GrafanaDataSourceStatus defines the observed state of GrafanaDataSource",
				Properties: map[string]spec.Schema{
					"grafanaPod": {
						SchemaProps: spec.SchemaProps{
							Description: "GrafanaPod is the grafana pod",
							Type:        []string{"array"},
							Items: &spec.SchemaOrArray{
								Schema: &spec.Schema{
									SchemaProps: spec.SchemaProps{
										Type:   []string{"string"},
										Format: "",
									},
								},
							},
						},
					},
					"status": {
						SchemaProps: spec.SchemaProps{
							Type:   []string{"string"},
							Format: "",
						},
					},
				},
				Required: []string{"status"},
			},
		},
		Dependencies: []string{},
	}
}
