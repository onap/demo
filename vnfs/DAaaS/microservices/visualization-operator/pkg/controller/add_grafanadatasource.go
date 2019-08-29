package controller

import (
	"visualization-operator/pkg/controller/grafanadatasource"
)

func init() {
	// AddToManagerFuncs is a list of functions to create controllers and add them to a manager.
	AddToManagerFuncs = append(AddToManagerFuncs, grafanadatasource.Add)
}
