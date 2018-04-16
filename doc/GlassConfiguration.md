# Glass3 Configuration

## Example
An example `glass_init.d` configuration file:
```json
{
  "Cmd": "Initialize",
  "HypoMax": 250,
  "PickMax": 10000,
  "SitePickMax": 30,
  "CorrelationMax": 1000,
  "PickDuplicateWindow": 2.5,
  "NumNucleationThreads": 5,
  "NumHypoThreads": 5,
  "NumWebThreads": 3,
  "SiteHoursWithoutPicking": 36,
  "SiteLookupInterval": 24,
  "Params": {
      "Thresh": 0.5,
      "Nucleate": 10,
      "sdAssociate": 3.0,
      "expAffinity": 2.5,
      "avgDelta": 0.0,
      "avgSigma": 0.75,
      "dCutFactor": 5.0,
      "dCutPercentage": 0.8,
      "dCutMin": 30.0,
      "iCycleLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow" : 22.5,
      "ReportThresh": 0.5,
      "ReportCut":5
  },
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./P.trv"
  },
  "AssociationPhases": [
      {
          "PhaseName": "P",
          "Range": [ 0, 0, 120, 180 ],
          "TravFile": "./P.trv"
      },
      {
          "PhaseName": "S",
          "Assoc": [ 10, 90 ],
          "TravFile": "./S.trv"
      },
      {
          "PhaseName": "PcP",
          "Range": [ 0, 0, 45 , 50],
          "TravFile": "./PcP.trv"
      },
      {
          "PhaseName": "PP",
          "Range": [ 90, 95, 175, 180 ],
          "TravFile": "./PP.trv"
      },
      {
          "PhaseName": "PKPab",
          "Range": [ 130, 132, 178, 180 ],
          "TravFile": "./PKPab.trv"
      },
      {
          "PhaseName": "PKPdf",
          "Range": [ 100, 114, 176, 180 ],
          "TravFile": "./PKPdf.trv"
      }
  ]
}
```

## General Configuration
These configuration parameters define overall glasscore variables. All values
should be greater than zero.

* **HypoMax** - The maximum size of the detection hypocenter list. Once the
maximum size is reached, glass will start expiring the oldest detections from
this list as new detections are made. This value is used for computational
performance tuning. Too small a value can impact glass association performance.
* **PickMax** - The maximum size of the input pick list. Once the maximum size
is reached, glass will start expiring the oldest picks from this list as new
picks are received. This value is used for computational performance tuning.
Too small a value can impact glass detection performance.
* **SitePickMax** - The maximum size of the per site pick list. This value is
used for computational performance tuning. Too small a value can impact glass
detection performance.
* **CorrelationMax** - The maximum size of the input correlation list. Once the
maximum size is reached, glass will start expiring the oldest correlations from
this list as new correlations are received. This value is used for computational
performance tuning.
* **PickDuplicateWindow** - The time window (+/-) within which to consider a
pick a duplicate of an existing pick from the same station.
* **NumNucleationThreads** - The number of nucleation/detection threads to run
in glass. This value should always be at least one. The upper limit depends
on local machine capabilities. This value is used for computational performance
tuning.
* **NumHypoThreads** - The number of hypocenter location threads to run in
glass. In general this value should be equal to **NumNucleationThreads** to
avoid race conditions. This value is used for computational performance tuning.
* **NumWebThreads** - The number of update threads to run per detection web in
glass. If the number of threads is zero, glass will halt while the updates are
processed.
* **SiteHoursWithoutPicking** - The amount of time, in hours, before a site will
be removed from the detection webs if a pick has not been made on that site. If
set to -1, sites will not be removed for not picking
* **SiteLookupInterval** - The amount of time, in hours, before a site will
request information via a SiteLookup message. A new site (previously unknown to
glass) will always request information once before this interval applies. If set
to -1, sites will not request information, even new ones.

## Nucleation Configuration
These configuration parameters define and control glasscore nucleation and
association. All values should be greater than zero, but exact limits are still
being determined.

* **Thresh** - The default viability threshold needed to exceed for a nucleation
to be successful. This value can be overridden in a detection grid (Web) if
provided as part of a specific grid configuration.
* **Nucleate** - The default number of data that need to be gathered to trigger
the nucleation of an event. This value can be overridden in a detection grid
(Web) if provided as part of a specific grid configuration.
* **sdAssociate** - The standard deviation cutoff used for associating a pick
with a hypocenter.
* **expAffinity** - The exponential factor used when calculating the affinity of
a pick with a hypocenter. **This value has little effect and likely does not need
to be changed.**
* **avgDelta** - The average station distance in degrees.  Used as the defining
value for a taper compensate for station density.
* **avgSigma** - The exponent of the gaussian weighting kernel in degrees.  It
is used to compensate for station density.
* **dCutFactor** - The distance factor used in calculating a hypocenter's
association distance cutoff.
* **dCutPercentage** - The percentage used to calculate a hypocenter's
association distance cutoff.
* **dCutMin** - The hypocenter's minimum association distance cutoff.
* **iCycleLimit** - The maximum number of processing cycles a hypocenter can run
without having new data associated.
* **CorrelationTimeWindow** - The time window (+/-) used to check for duplicate
correlations and to associate a correlation with an existing hypocenter.
* **CorrelationDistanceWindow** - The distance window (+/-) used to check for
duplicate correlations and to associate a correlation with an existing
hypocenter.
* **CorrelationCancelAge** - The minimum age of a correlations before allowing a
hypocenter to cancel.
* **ReportThresh** The viability threshold needed to exceed to report a
hypocenter. Defaults to **Thresh**.
* **ReportCut** The default number of data that need to be associated to report
a hypocenter. Defaults to **Nucleate**.

### DefaultNucleationPhase
This parameter defines the default nucleation phase for glass. This value can be
overridden in a detection grid (Web) if provided as part of a specific grid
configuration.
* **PhaseName** - The name of the default nucleation phase
* **TravFile** - The path to the travel-time lookup file for the default
nucleation phase.

### Association Phases
A list of phases to use for association.
* **PhaseName** - The name of the association phase
* **Assoc** - The association range [Minimum, Maximum] for this phase.  Mutually
exclusive with **Range**
* **Range** - The association taper [Start, RampUpEnd, RampDownStart, End] for
this association phase.  Mutually exclusive with **Assoc**
* **TravFile** - The path to the travel-time lookup file for the association
phase.

## Grid Configuration
GLASS 3 uses detection grids (or webs) of nodes to nucleate detections. In
general, there are two types of grids, Regional/Local grids, and Global grids.

## General Parameters
These configuration parameters are common to all GLASS 3 Detection grids.

* **Name** - The name of this detection grid
* **Thresh** - The viability threshold needed to exceed for a nucleation in a
grid to be successful.  Overrides the default **Thresh** in Nucleation
Parameters
* **Nucleate** - The default number of data that need to be gathered for a
nucleation in a grid to be successful.  Overrides the default **Nucleate** in
Nucleation Parameters
* **Detect** - The number of closest stations to each detection node to use in
a grid.
* **NucleationPhases** - The one or two phases to use for nucleation in a grid,
if not present, the grid will use the DefaultNucleationPhase
	* **Phase1** - The primary nucleation phase for a grid
		* **PhaseName** - The name of the primary nucleation phase
		* **TravFile** - The path to the travel-time lookup file for the primary
    nucleation phase.
	* **Phase2** - The optional secondary nucleation phase for a grid, generally
  only used in regional and local grids.
		* **PhaseName** - The name of the secondary nucleation phase
		* **TravFile** - The path to the travel-time lookup file for the secondary
    nucleation phase.
* **Nets** - A list of network codes to filter the stations available to a grid.
* **Sites** - A list of SCNL codes to filter the stations available to a grid,
generally used only in very small local grids.
* **UseOnlyTeleseismicStations** - A flag indicating that the grid should only
use stations flagged as "UseForTeleseismic" in the station list, generally used
only in global grids.
* **Z** - The list of depth layers for a grid
* **Resolution** - The desired inter-node resolution (or spacing) for a grid.
* **SaveGrid** - A flag indicating whether to save the grid node locations to a
file for evaluation.
* **Update** - A flag indicating whether a grid is allowed to add or remove sites
from nodes .

## Regional / Local Grid
This is a detection grid designed to cover some regional or local area of
interest, with the provided depth layers.
### Example:
```json
{
	"Cmd": "Grid",
	"Name": "Oklahoma",
	"Thresh": 0.5,
	"Nucleate": 6,
	"Detect": 10,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		},
	    "Phase2": {
      		"PhaseName": "S",
      		"TravFile": "./S.trv"
  		}
	},
	"Nets": ["OK", "US", "N4"],
  "UseOnlyTeleseismicStations": false,
	"Lat": 36.0,
	"Lon": -97.5,
	"Z": [ 10.0, 30.0 ],
	"Resolution": 25.0,
	"Rows": 51,
	"Cols": 51,
	"SaveGrid": true
}
```
### Parameters
These parameters are specific to Regional / Local Grids.
* **Lat** - The latitude of the center point of this detection grid
* **Lon** - The longitude of the center point of this detection grid
* **Rows** - The number of rows (height) of this detection grid in nodes.
* **Cols** - The number of columns (width) of this detection grid in nodes.

## Global Grid
This is a detection grid that is designed to cover the globe with equally spaced
detection nodes at the provided depth layers.
### Example
```json
{
	"Cmd": "Global",
	"Name": "Global",
	"Thresh": 0.5,
	"Nucleate": 6,
	"Detect": 24,
	"NucleationPhases":
	{
		"Phase1":
		{
			"PhaseName": "P",
			"TravFile": "./P.trv"
		},
		"Phase2":
		{
			"PhaseName": "S",
			"TravFile": "./S.trv"
		}
	},
	"Nets": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AU", "MX",
    "AT"],
  "UseOnlyTeleseismicStations": true,
	"Z": [10.0, 30.0, 50.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 750.0],
	"Resolution": 100.0,
	"SaveGrid": false
}
```
