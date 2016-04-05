module.exports = function(mongoose) {

    // use mongoose to declare a user schema
    var UserSchema = mongoose.Schema({
        username: String,
        password: String,
        firstName: String,
        lastName: String,
        emails: [String],
        phones: [String],
        pcp: [String]
        // collection property sets
        // collection name to 'user'
    }, {collection: 'capstone.user'});
    return UserSchema;
};

