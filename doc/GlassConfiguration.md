# Glass3 Configuration

## Example
An example `glass_init.d` configuration file:
```json
{
  "Cmd": "Initialize",
  "MaximumNumberOfPicks": 10000,
  "MaximumNumberOfCorrelations": 1000,
  "MaximumNumberOfHypos": 250,
  "PickDuplicateWindow": 2.5,
  "NumberOfNucleationThreads": 5,
  "NumberOfHypoThreads": 5,
  "NumberOfWebThreads": 1,
  "SiteHoursWithoutPicking": 36,
  "SiteLookupInterval": 24,
  "SiteMaximumPicksPerHour": 200,
  "AllowPickUpdates": false,
  "Params": {
      "NucleationStackThreshold": 0.5,
      "NucleationDataCountThreshold": 10,
      "AssociationStandardDeviationCutoff": 3.0,
      "PruningStandardDeviationCutoff": 3.0,
      "NonLocatingPhaseCutoffFactor": 3.0,
      "PickAffinityExponentialFactor": 2.5,
      "DistanceCutoffFactor": 5.0,
      "DistanceCutoffPercentage": 0.8,
      "DistanceCutoffMinimum": 30.0,
      "HypoProcessCountLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow" : 22.5,
      "HypocenterTimeWindow": 30.0,
      "HypocenterDistanceWindow": 3.0,
      "ReportingStackThreshold": 0.5,
      "ReportingDataThreshold": 5,
      "EventFragmentDepthThreshold": 550.0,
      "EventFragmentAzimuthThreshold": 270.0
  },
  "PickClassification":{
      "NoiseThreshold": 0.75,
      "PhaseThreshold": 0.75
  },
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./P.trv"
  },
  "AssociationPhases": [
      {
          "PhaseName": "P",
          "Assoc": [ 0, 100 ],
          "TravFile": "./P.trv"
      },
      {
          "PhaseName": "S",
          "Assoc": [ 10, 90 ],
          "TravFile": "./S.trv"
      },
      {
          "PhaseName": "PcP",
          "Assoc": [ 0, 50],
          "TravFile": "./PcP.trv"
      },
      {
          "PhaseName": "PP",
          "Assoc": [ 90, 180 ],
          "TravFile": "./PP.trv"
      },
      {
          "PhaseName": "PKPab",
          "Assoc": [ 130, 180 ],
          "TravFile": "./PKPab.trv"
      },
      {
          "PhaseName": "PKPdf",
          "Assoc": [ 100, 180 ],
          "TravFile": "./PKPdf.trv",
          "UseForLocation": true,
          "PublishPhase": true
      },
      {
          "PhaseName": "PKKPab",
          "Assoc": [ 100, 125 ],
          "TravFile": "./PKKPab.trv",
          "UseForLocation": false,
          "PublishPhase": false
      }
  ]
}
```

## General Configuration
These configuration parameters define overall glasscore variables. All values
should be greater than zero.

* **MaximumNumberOfHypos** - The maximum size of the detection hypocenter list. Once the
maximum size is reached, glass will start expiring the oldest detections from
this list as new detections are made. This value is used for computational
performance tuning. Too small a value can impact glass association performance.
* **MaximumNumberOfPicks** - The maximum size of the input pick list. Once the maximum size
is reached, glass will start expiring the oldest picks from this list as new
picks are received. This value is used for computational performance tuning.
Too small a value can impact glass detection performance.
* **MaximumNumberOfCorrelations** - The maximum size of the input correlation list. Once the
maximum size is reached, glass will start expiring the oldest correlations from
this list as new correlations are received. This value is used for computational
performance tuning.
* **PickDuplicateWindow** - The time window (+/-) within which to consider a
pick a duplicate of an existing pick from the same station.
* **NumberOfNucleationThreads** - The number of nucleation/detection threads to run
in glass. This value should always be at least one. The upper limit depends
on local machine capabilities. This value is used for computational performance
tuning.
* **NumberOfHypoThreads** - The number of hypocenter location threads to run in
glass. In general this value should be equal to **NumberOfNucleationThreads** to
avoid race conditions. This value is used for computational performance tuning.
* **NumberOfWebThreads** - The number of update threads to run per detection web in
glass. If the number of threads is zero, glass will halt while the updates are
processed. This value is used for computational performance tuning.
* **SiteHoursWithoutPicking** - The amount of time, in hours, before a site will
be removed from the detection webs if a pick has not been made on that site. If
set to -1, sites will not be removed for not picking
* **SiteLookupInterval** - The amount of time, in hours, before a site will
request information via a SiteLookup message. A new site (previously unknown to
glass) will always request information once before this interval applies. If set
to -1, sites will not request information, even new ones.
* **SiteMaximumPicksPerHour** - The number of picks per hour, above which a site
will be removed from the detection webs  If set to -1, sites will not be removed
from the detection webs due to pick rate.
* **AllowPickUpdates** - A boolean flag indicating whether glass should glass
should reject new duplicate picks of an existing pick (false) or update an
existing pick with the latest duplicate pick (true).

## Nucleation Configuration
These configuration parameters define and control glasscore nucleation and
association. All values should be greater than zero, but exact limits are still
being determined.

* **NucleationStackThreshold** - The default threshold that the bayesian stack
needs to exceed for a nucleation to be declared. This threshold is also used to
cancel a previously nucleated hypocenter. This value can be overridden in a
detection grid (Web).
* **NucleationDataCountThreshold** - The default threshold that the number of supporting  
data (eg. Picks, Correlations) must exceed for a nucleation to be declared.  
This threshold is also used to cancel a previously nucleated hypocenter. This
value can be overridden in a detection grid (Web).
* **AssociationStandardDeviationCutoff** - The cutoff threshold in terms of number
of standard deviations for associating data (eg. Picks) with a hypocenter. The
standard deviation is fixed to 1.
* **PruningStandardDeviationCutoff** - The cutoff threshold in terms of number
of standard deviations for pruning data (eg. Picks) from a hypocenter. The
standard deviation is fixed to 1.
* **NonLocatingPhaseCutoffFactor** - Thee multiplicative factor used to expand
the SD cutoff to keep data associated to a hypocenter when the Pick phase
does not affect the location
* **PickAffinityExponentialFactor** - The exponential factor used when calculating the affinity of
a pick with a hypocenter. **This value has little effect and likely does not need
to be changed.**
* **DistanceCutoffFactor** - The factor, used with the distance cutoff ratio to
calculate a hypocenter's association distance cutoff, i.e the distance beyond
which data (eg. Picks) will not be associated.
* **DistanceCutoffRatio** - The ratio, used with the distance cutoff factor to
calculate a hypocenter's association distance cutoff, i.e the distance beyond
which data (eg. Picks) will not be associated.
* **DistanceCutoffMinimum** - The hypocenter's minimum association distance cutoff.
* **HypoProcessCountLimit** - The maximum number of processing cycles a hypocenter can run
without having new data associated.
* **CorrelationTimeWindow** - The time window (+/-) used to check for duplicate
correlations and to associate a correlation with an existing hypocenter.
* **CorrelationDistanceWindow** - The distance window (+/-) used to check for
duplicate correlations and to associate a correlation with an existing
hypocenter.
* **CorrelationCancelAge** - The minimum age of a correlations before allowing a
hypocenter to cancel.
* **HypocenterTimeWindow** - The time window (+/-) used to check for mergeable
hypocenters in seconds.
* **HypocenterDistanceWindow** - The distance window (+/-) used to check for
mergeable hypocenters in degrees.
* **ReportingStackThreshold** The viability threshold needed to exceed to report a
hypocenter. Defaults to **NucleationStackThreshold**.
* **ReportingDataThreshold** The default number of data that need to be associated to report
a hypocenter. Defaults to **NucleationDataCountThreshold**.
* **EventFragmentDepthThreshold** The depth threshold for declaring a hypo an
event fragment, in combination with  **EventFragmentAzimuthThreshold**.
* **EventFragmentAzimuthThreshold** The azimuth threshold for declaring a hypo an
event fragment, in combination with **EventFragmentDepthThreshold**.

## Pick Classification Configuration
These configuration parameters define the thresholds to use when glass processes
picks with optional classification information from external algorithms

* **NoiseThreshold** - The threshold used by glass when determining whether it
accepts the classification of a new pick as "noise". Noise picks are not processed
* **PhaseThreshold** - The threshold used by glass when determining whether it
accepts the classification of the phase of a new pick. Classified picks are
nucleated/associated according to the given phase.

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
* **Assoc** - The association distance range [Minimum, Maximum] for this phase. 
* **TravFile** - The path to the travel-time lookup file for the association
phase.
* **UseForLocation** - An optional flag indicating whether this association phase 
can be used when generating locations, default is true.
* **PublishPhase** - An optional flag indicating whether this association phase
can be published when generating output detection messages, default is true.

## Grid Configuration
GLASS 3 uses detection grids (or webs) of nodes to nucleate detections. In
general, there are two types of grids, Regional/Local grids, and Global grids.

## General Parameters
These configuration parameters are common to all GLASS 3 Detection grids.

* **Name** - The name of this detection grid
* **NucleationStackThreshold** - The viability threshold needed to exceed for a nucleation in a
grid to be successful.  Overrides the default **NucleationStackThreshold** in Nucleation
Parameters
* **NucleationDataCountThreshold** - The default number of data that need to be gathered for a
nucleation in a grid to be successful.  Overrides the default **NucleationDataCountThreshold** in
Nucleation Parameters
* **NumStationsPerNode** - The number of closest stations to each detection node to use in
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
* **IncludeNetworks** - A list of network codes limit the stations available to
a grid, a station must have one of the given network codes to be used in the grid
* **IncludeSites** - A list of SCNL codes to limit the stations available to
a grid, a station must have one of the given SCNL codes to be used in the grid
* **UseOnlyTeleseismicStations** - A flag indicating that the grid should only
use stations flagged as "UseForTeleseismic" in the station list, generally used
only in global grids.
* **DepthLayers** - The list of depth layers for a grid in kilometers
* **MaximumDepth** - The web specific maximum allowable locator depth in
kilometers
* **AzimuthGapTaper** - The web specific value in degrees where the locator
should start down weighting for azimuthal gap
* **NodeResolution** - The desired inter-node resolution (or spacing) for a grid
in kilometers.
* **ZoneStatsFile** - An optional file containing zonestats information to be used
when generating a grid to prevent nodes in areas where deep seismicity does not
occur from being generated
* **DepthResolution** - An optional maximum depth in kilometers below which a
trigger at a specific node is rejected. If not provided, the maximum depth
defaults to **MaximumDepth**
* **SaveGrid** - A flag indicating whether to save the grid node locations to a
file for evaluation.
* **UpdateGrid** - A flag indicating whether a grid is allowed to add or remove sites
from nodes. Note that if Update is false, features like **SiteHoursWithoutPicking**
and **SiteLookupInterval** will be omitted for this grid.

## Regional / Local Grid
This is a detection grid designed to cover some regional or local area of
interest, with the provided depth layers.
### Example:
```json
{
	"Cmd": "Grid",
	"Name": "Oklahoma",
	"NucleationStackThreshold": 0.5,
	"NucleationDataCountThreshold": 6,
	"NumStationsPerNode": 10,
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
	"IncludeNetworks": ["OK", "US", "N4"],
  "UseOnlyTeleseismicStations": false,
	"CenterLatitude": 36.0,
	"CenterLongitude": -97.5,
	"DepthLayers": [ 10.0, 30.0 ],
  "MaximumDepth": 900.0,
  "AzimuthGapTaper": 360.0,
	"NodeResolution": 25.0,
	"NumberOfRows": 51,
	"NumberOfColumns": 51,
  "ZoneStatsFile": "./qa_zonestats.txt",
  "DepthResolution": 125.0,  
	"SaveGrid": true,
  "UpdateGrid": true
}
```
### Parameters
These parameters are specific to Regional / Local Grids.
* **CenterLatitude** - The latitude of the center point of this detection grid
* **CenterLongitude** - The longitude of the center point of this detection grid
* **NumberOfRows** - The number of rows (height) of this detection grid in nodes.
* **NumberOfColumns** - The number of columns (width) of this detection grid in nodes.

## Global Grid
This is a detection grid that is designed to cover the globe with equally spaced
detection nodes at the provided depth layers.
### Example
```json
{
	"Cmd": "Global",
	"Name": "Global",
	"NucleationStackThreshold": 0.5,
	"NucleationDataCountThreshold": 6,
	"NumStationsPerNode": 24,
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
	"IncludeNetworks": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AU", "MX", "AT"],
  "UseOnlyTeleseismicStations": true,
	"DepthLayers": [10.0, 30.0, 50.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 750.0],
  "MaximumDepth": 900.0,
  "AzimuthGapTaper": 360.0,
	"NodeResolution": 100.0,
  "ZoneStatsFile": "./qa_zonestats.txt",
  "DepthResolution": 125.0,
	"SaveGrid": false,
  "UpdateGrid": true
}
```
