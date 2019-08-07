package controller

import (
	"demo/vnfs/DAaaS/microservices/collectd-operator/pkg/controller/collectdglobal"
)

func init() {
	// AddToManagerFuncs is a list of functions to create controllers and add them to a manager.
	AddToManagerFuncs = append(AddToManagerFuncs, collectdglobal.Add)
}
