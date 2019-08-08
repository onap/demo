package controller

import (
	"collectd-operator/pkg/controller/collectdglobal"
)

func init() {
	// AddToManagerFuncs is a list of functions to create controllers and add them to a manager.
	AddToManagerFuncs = append(AddToManagerFuncs, collectdglobal.Add)
}
