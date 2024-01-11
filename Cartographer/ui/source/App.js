enyo.kind({
	name: "App",
	kind: "FittableRows",
	fit: true,
	components:[
		{name: "ProgressPopup",
		floating: true,
		centered: true,
		kind: "onyx.Popup",
		style: "width: 50%",
		components:[
			{name: "ProgressTitle", content: "Sending Download Request..."},
			{name: "ProgressBar", kind: "onyx.ProgressBar", min: 0, max: 100},
		]},
		
		{kind: "onyx.Toolbar", content: "Cartographer"},
		{kind: "Map", style: "width: 100%; height: 100%;", fit: true},
		{kind: "onyx.Toolbar", components: [
			{content: "Pan and Zoom to select an area"},
			{name: "GenerateButton",
			kind: "onyx.Button",
			style: "float: right;",
			content: "Generate",
			ontap: "generateTapped"}
		]},
	],
	generateTapped: function() {
		if(window.cartographer) {
			this.$.ProgressPopup.show();
			this.$.GenerateButton.setDisabled(true);
			
			var bounds = this.$.map.map.getBounds();
			window.cartographer.Bubble(
				JSON.stringify({
					type: "latLon",
					minlon: bounds._southWest.lng,
					minlat: bounds._southWest.lat,
					maxlon: bounds._northEast.lng,
					maxlat: bounds._northEast.lat
				})
			);
		} else {
			enyo.log(window.enyo);
			enyo.log("Running in-browser, backend interface not found");
		}
	},
	cWaterfall: function(inJson) {
		if(inJson.progress) {
			this.$.ProgressBar.setProgress(inJson.progress);
			if(inJson.progress == 100) {
				var storedThis = this;
				setTimeout(this.processingComplete, 1500);
			}
		}
		if(inJson.message) {
			this.$.ProgressTitle.setContent(inJson.message);
		}
	},
	processingComplete: function() {
			window.cartographer.Bubble(
				JSON.stringify({ type: "pickFile" })
			);
			window.enyo.$.app.$.ProgressPopup.hide();
			window.enyo.$.app.$.GenerateButton.setDisabled(false);
	}
});
