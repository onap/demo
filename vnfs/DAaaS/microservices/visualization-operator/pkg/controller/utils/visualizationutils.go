package visualizationutils

// Define the GrafanaDatasource finalizer for handling deletion
const (
	VisualizationFinalizer = "finalizer.visualization.onap.org"
)

// Contains checks if a string is contained in a list of strings
func Contains(list []string, s string) bool {
	for _, v := range list {
		if v == s {
			return true
		}
	}
	return false
}

// Remove checks and removes a string from a list of strings
func Remove(list []string, s string) []string {
	for i, v := range list {
		if v == s {
			list = append(list[:i], list[i+1:]...)
		}
	}
	return list
}
