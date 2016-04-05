module.exports = function(mongoose) {

    // use mongoose to declare a user schema
    var bpSchema = mongoose.Schema({
        title: String,
        date: Date,
        userId: String,
        diastolic: String,
        systolic: String
        // collection property sets
        // collection name to 'user'
    }, {collection: 'capstone.bp'});
    return bpSchema;
};

