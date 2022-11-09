/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ['./dist/*.html', './dist/*.js', './node_modules/tw-elements/dist/js/**/*.js'],
  theme: {
    extend: {},
  },
  plugins: [
    require('tw-elements/dist/plugin')
  ],
}
