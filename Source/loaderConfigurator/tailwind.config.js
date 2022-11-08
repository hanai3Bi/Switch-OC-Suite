/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ['./src/*.html', './node_modules/tw-elements/dist/js/**/*.js'],
  theme: {
    extend: {},
  },
  plugins: [
    require('tw-elements/dist/plugin')
  ],
}
