/** @type {import('tailwindcss').Config} */
export default {
	content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
	theme: {
		extend: {
			colors: {
				primary: "#00F38A",
			},
			// font
			fontFamily: {
				bigNoodle: ["BigNoodleTitling"],
				terminal: ["BigBlue_Terminal_437TT"],
				deg7: ["DSEG7"],
			},
		},
	},
	plugins: [],
};
