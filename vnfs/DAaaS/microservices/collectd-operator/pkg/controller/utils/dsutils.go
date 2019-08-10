package utils

import (
	"path/filepath"
	"strings"
	"strconv"

	onapv1alpha1 "collectd-operator/pkg/apis/onap/v1alpha1"

	corev1 "k8s.io/api/core/v1"
	extensionsv1beta1 "k8s.io/api/extensions/v1beta1"
	logf "sigs.k8s.io/controller-runtime/pkg/runtime/log"
)

var log = logf.Log.WithName("dsutils")

const (
	collectdContainerName = "collectd"

	// canonical label for the volume created for TypesDB
	// reason - a DNS-1123 label must consist of lower case alphanumeric characters
	//			or '-', and must start and end with an alphanumeric character
	typesDB = "types-db"
)

// RemoveTypesDB - removes TypesDB volumes and volume mounts from collectd pods.
func RemoveTypesDB(ds *extensionsv1beta1.DaemonSet) {
	vols := &ds.Spec.Template.Spec.Volumes
	for i:=0; i < len(*vols); i++ {
		if (*vols)[i].Name == typesDB {
			*vols = append((*vols)[:i], (*vols)[i+1:]...)
			i--
		}
	}

	containers := &ds.Spec.Template.Spec.Containers
	for j, container := range *containers {
		if container.Name == collectdContainerName {
			vms := &(*containers)[j].VolumeMounts
			for i:=0; i < len(*vms); i++ {
				if (*vms)[i].Name == typesDB {
					*vms = append((*vms)[:i], (*vms)[i+1:]...)
					i--
				}
			}
		}
	}
}

// UpsertTypesDB - Insert/Update TypesDB volumes and volume mounts to collectd pods.
func UpsertTypesDB(ds *extensionsv1beta1.DaemonSet, cm *corev1.ConfigMap, cr *onapv1alpha1.CollectdGlobal) {
	typesVM := findMountInfo(cr)
	if *typesVM == nil || len(*typesVM) == 0 {
		return
	}
	typesDBVolume := &corev1.ConfigMapVolumeSource{
		LocalObjectReference: corev1.LocalObjectReference{Name: cm.Name},
	}
	vols := &ds.Spec.Template.Spec.Volumes
	var hasUpdated bool
	for i, vol := range *vols {
		// Update case
		if vol.Name == typesDB {
			(*vols)[i].ConfigMap = typesDBVolume
			hasUpdated = true
		}
	}

	if !hasUpdated {
		//Insert case
		*vols = append(*vols, corev1.Volume{
			Name: typesDB,
			VolumeSource: corev1.VolumeSource{
				ConfigMap: typesDBVolume,
			},
		})
	}

	containers := &ds.Spec.Template.Spec.Containers

	for j, container := range *containers {
		if container.Name == collectdContainerName {
			vms := &(*containers)[j].VolumeMounts
			for i:=0; i < len(*vms); i++ {
				// Update case (Equivalent to remove and add)
				if (*vms)[i].Name == typesDB {
					*vms = append((*vms)[:i], (*vms)[i+1:]...)
					i--
				}
			}

			*vms = append(*vms, *typesVM...)
		}
	}
}

func findMountInfo(cr *onapv1alpha1.CollectdGlobal) *[]corev1.VolumeMount {
	log.V(1).Info(":::::Entering findMountInfo:::::")
	var typesVM []corev1.VolumeMount
	globalOpts := strings.Split(cr.Spec.GlobalOptions, "\n")
	log.V(1).Info(":::::findMountInfo:::::", "GlobalOptions", globalOpts)
	for i, globalOpt := range globalOpts {
		log.V(1).Info(":::::For Loop:::::", "Item No:", i, "LineEntry:", globalOpt)
		s := strings.Fields(globalOpt)
		log.V(1).Info(":::::s:::::", "s:", s)
		if s != nil && len(s) != 0 && s[0] == "TypesDB" {
			path,_ := strconv.Unquote(s[1])
			_, file := filepath.Split(path)
			log.V(1).Info(":::::file:::::", "s[1]:", path, "file:", file)
			vm := corev1.VolumeMount{Name: typesDB, MountPath: path, SubPath: file}
			typesVM = append(typesVM, vm)
			log.V(1).Info(":::::TypesVM:::::", "TypesVM:", typesVM)
		}
	}
	log.V(1).Info(":::::Exiting findMountInfo:::::")
	return &typesVM
}
