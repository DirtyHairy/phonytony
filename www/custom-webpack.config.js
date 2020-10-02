const MiniCssExtractPlugin = require('mini-css-extract-plugin');

module.exports = {
    output: {
        filename: '[name].[contenthash:8].js',
    },
    plugins: [new MiniCssExtractPlugin({ filename: `[name].[contenthash:8].css` })],
};
